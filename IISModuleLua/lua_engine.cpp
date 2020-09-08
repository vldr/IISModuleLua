#include "shared.h"

typedef struct _LuaEngine
{
	lua_State* L;
	HANDLE mutex_handle;
	HANDLE directory_changes_handle;
	char file_path[MAX_PATH];

	SLIST_ENTRY* list_entry;
} LuaEngine;

int 
lua_engine_printf(const char* format, ...)
{
	char str[1024];

	va_list argptr;
	va_start(argptr, format);
	int ret = vsnprintf(str, sizeof(str), format, argptr);
	va_end(argptr);

	OutputDebugStringA(str);

	return ret;
}

static int 
lua_engine_register(lua_State* L)  
{
	lua_stack_guard(L, 0);

	luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_pushvalue(L, 1);

	lua_setfield(L, LUA_REGISTRYINDEX, "lua_engine_begin_request");

	return 0;
}

static int 
lua_engine_print(lua_State* L)
{
	lua_stack_guard(L, 0);

	int n = lua_gettop(L);
	int i;

	lua_getglobal(L, "tostring");

	for (i = 1; i <= n; i++)
	{
		const char* s;
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_call(L, 1, 1);

		s = lua_tostring(L, -1);

		if (s == NULL)
		{
			return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
		}

		if (i > 1) OutputDebugStringA("\t");

		OutputDebugStringA(s);

		lua_pop(L, 1);
	}

	OutputDebugStringA("\n");

	lua_pop(L, 1);

	return 0;
}

static int 
lua_engine_http_newindex(lua_State* L)
{
	return luaL_error(L, "attempt to update a read-only table");
}

static bool 
lua_engine_lock(LuaEngine* lua_engine)
{
	assert(lua_engine != nullptr);
	assert(lua_engine->mutex_handle != nullptr);

	DWORD retvalue = WAIT_FAILED;

	if (lua_engine && lua_engine->mutex_handle)
	{
		retvalue = WaitForSingleObject(lua_engine->mutex_handle, INFINITE);
	}

	return retvalue == WAIT_OBJECT_0;
}

static bool 
lua_engine_unlock(LuaEngine* lua_engine)
{	
	assert(lua_engine != nullptr);
	assert(lua_engine->mutex_handle != nullptr);

	return lua_engine 
		&& lua_engine->mutex_handle 
		&& !ReleaseMutex(lua_engine->mutex_handle);
}

static void 
lua_engine_register_http(lua_State* L)
{
	assert(L != nullptr);

	if (L)
	{
		lua_stack_guard(L, 0);

		struct constant_pair { const char* key; int val; };
		struct constant_pair constants[] =
		{
			{ "Continue", 0 },
			{ "Finish", 1 },
			{ 0, 0 }
		};

		lua_newtable(L);

		luaL_newmetatable(L, "HttpMetatable");

		lua_pushstring(L, "__index");
		lua_newtable(L);
		for (struct constant_pair* p = constants; p->key != 0; p++)
		{
			lua_pushstring(L, p->key);
			lua_pushnumber(L, p->val);
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);

		lua_pushstring(L, "__newindex");
		lua_pushcfunction(L, lua_engine_http_newindex);
		lua_rawset(L, -3);

		lua_setmetatable(L, -2);
		lua_setglobal(L, "http");
	}
}

static lua_State* 
lua_engine_new_lua_state()
{
	lua_State* L = luaL_newstate();

	assert(L != nullptr);

	if (L)
	{
		luaL_openlibs(L);

		lua_engine_register_http(L);

		lua_response_register(L);
		lua_request_register(L);

		lua_register(L, "print", lua_engine_print);
		lua_register(L, "register", lua_engine_register);
	}

	return L;
}

static void CALLBACK 
lua_engine_watch_callback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	assert(lpParameter != nullptr);

	LuaEngine* lua_engine = (LuaEngine*)lpParameter;

	if (lua_engine && lua_engine->L && lua_engine_lock(lua_engine))
	{
		lua_engine_printf("detected changes, reloading script\n");

		lua_close(lua_engine->L);
		lua_engine->L = lua_engine_new_lua_state();

		if (lua_engine_load_file(lua_engine->L, lua_engine->file_path) != 0)
		{
			lua_engine_printf("%s\n", lua_tostring(lua_engine->L, -1));
			lua_pop(lua_engine->L, 1);
		}

		if (FindNextChangeNotification(lua_engine->directory_changes_handle))
		{
			HANDLE registration_handle;
			BOOL result = RegisterWaitForSingleObject(
				&registration_handle,
				lua_engine->directory_changes_handle,
				&lua_engine_watch_callback,
				(void*)lua_engine,
				INFINITE,
				WT_EXECUTEONLYONCE
			);

			if (!result)
			{
				lua_engine_printf("failed to register a wait for further directory changes\n");

				CloseHandle(lua_engine->directory_changes_handle);
				lua_engine->directory_changes_handle = nullptr;
			}
		}
		else
		{
			lua_engine_printf("failed to register a notification for further directory changes\n");

			CloseHandle(lua_engine->directory_changes_handle);
			lua_engine->directory_changes_handle = nullptr;
		}

		lua_engine_unlock(lua_engine);
	}
}

static void 
lua_engine_load_and_watch(
	LuaEngine* lua_engine, 
	const wchar_t* name
)
{
	assert(lua_engine != nullptr);
	assert(name != nullptr);

	lua_stack_guard(lua_engine->L, 0);

	wchar_t* documents_path = nullptr;

	HRESULT hr = SHGetKnownFolderPath(
		FOLDERID_Public,
		0,
		nullptr,
		&documents_path
	);

	if (SUCCEEDED(hr) && documents_path)
	{
		wchar_t directory_to_watch[MAX_PATH];
		swprintf_s(directory_to_watch, L"%s\\%s", documents_path, name);

		HANDLE directory_changes_handle = FindFirstChangeNotificationW(
			directory_to_watch,
			TRUE,
			FILE_NOTIFY_CHANGE_LAST_WRITE
		);

		if (directory_changes_handle != INVALID_HANDLE_VALUE)
		{
			lua_engine->directory_changes_handle = directory_changes_handle;
			sprintf_s(lua_engine->file_path, "%ls\\%ls\\%ls.lua", documents_path, name, name);

			HANDLE registration_handle;
			BOOL result = RegisterWaitForSingleObject(
				&registration_handle,
				lua_engine->directory_changes_handle,
				&lua_engine_watch_callback,
				(void*)lua_engine,
				INFINITE,
				WT_EXECUTEONLYONCE
			);

			if (result)
			{
				if ((luaL_loadfile(lua_engine->L, lua_engine->file_path) || lua_pcall(lua_engine->L, 0, 0, 0)) != 0)
				{
					lua_engine_printf("%s\n", lua_tostring(lua_engine->L, -1));
					lua_pop(lua_engine->L, 1);
				}
			} 
			else
			{
				lua_engine_printf("failed to register for directory changes\n");

				CloseHandle(lua_engine->directory_changes_handle);
				lua_engine->directory_changes_handle = nullptr;
			}
		}
		else
		{
			lua_engine_printf("failed to initially register for directory changes\n");
		}

		CoTaskMemFree(documents_path);
		documents_path = nullptr;
	}
}

REQUEST_NOTIFICATION_STATUS
lua_engine_begin_request(
	LuaEngine* lua_engine,
	IHttpContext* http_context
)
{
	assert(lua_engine != nullptr);
	assert(http_context != nullptr);

	REQUEST_NOTIFICATION_STATUS result = RQ_NOTIFICATION_CONTINUE;

	if (!lua_engine)
	{
		lua_engine_printf("call to begin request failed\n");
		return result;
	}

	lua_State* L = lua_engine->L;

	assert(L != nullptr);

	if (L && lua_engine_lock(lua_engine))
	{  
		lua_stack_guard(L, 0);
		lua_getfield(L, LUA_REGISTRYINDEX, "lua_engine_begin_request");

		if (lua_isfunction(L, -1))
		{
			ResponseLua* response_lua = lua_response_push(L);
			RequestLua* request_lua = lua_request_push(L);
			 
			response_lua->http_context = http_context;
			response_lua->http_response = http_context->GetResponse();
			 
			request_lua->http_context = http_context;
			request_lua->http_request = http_context->GetRequest();

			if (lua_pcall(L, 2, 1, 0) == 0)
			{
				result = (REQUEST_NOTIFICATION_STATUS)(int)lua_tonumber(L, -1);
			}
			else
			{
				lua_engine_printf("%s\n", lua_tostring(L, -1));
			}

			response_lua->http_context = nullptr;
			response_lua->http_response = nullptr;

			request_lua->http_context = nullptr;
			request_lua->http_request = nullptr;
		}
		
		lua_pop(L, 1);
		lua_engine_unlock(lua_engine);
	}

	return result ? RQ_NOTIFICATION_FINISH_REQUEST : RQ_NOTIFICATION_CONTINUE;
}

LuaEngine* 
lua_engine_create(const wchar_t* name)
{
	assert(name != nullptr);

	LuaEngine* lua_engine = nullptr;
	lua_State* L = nullptr;
	HANDLE mutex_handle = nullptr;

	//////////////////////////////////////////

	mutex_handle = CreateMutex(NULL, FALSE, NULL);

	assert(mutex_handle != nullptr);

	if (!mutex_handle)
	{
		lua_engine_printf("failed to create mutex handle\n");
		goto error;
	}

	//////////////////////////////////////////

	L = lua_engine_new_lua_state();

	if (!L)
	{
		lua_engine_printf("failed to create lua state\n");
		goto error;
	}

	//////////////////////////////////////////

	lua_engine = (LuaEngine*)malloc(sizeof(*lua_engine));

	assert(lua_engine != nullptr);

	if (!lua_engine)
	{
		lua_engine_printf("failed to create lua engine object\n");
		goto error;
	}

	lua_engine->L = L;
	lua_engine->mutex_handle = mutex_handle;
	lua_engine->list_entry = nullptr;

	////////////////////////////////////////

	lua_engine_load_and_watch(lua_engine, name);

	goto finish;

error:
	if (L)
	{
		lua_close(L);
		L = nullptr;
	}

	if (lua_engine)
	{
		free(lua_engine);
		lua_engine = nullptr;
	}

	if (mutex_handle)
	{
		CloseHandle(mutex_handle);
		mutex_handle = nullptr;
	}

finish:
	return lua_engine;
}

LuaEngine* 
lua_engine_destroy(LuaEngine* lua_engine)
{
	assert(lua_engine != nullptr);
	assert(lua_engine->L != nullptr);
	assert(lua_engine->mutex_handle != nullptr);
	assert(lua_engine->list_entry != nullptr);

	if (lua_engine)
	{
		if (lua_engine->L)
		{
			lua_close(lua_engine->L);
			lua_engine->L = nullptr;
		}

		if (lua_engine->mutex_handle)
		{
			CloseHandle(lua_engine->mutex_handle);
			lua_engine->mutex_handle = nullptr;
		}

		if (lua_engine->list_entry)
		{
			_aligned_free(lua_engine->list_entry);
			lua_engine->list_entry = nullptr;
		}

		free(lua_engine);
		lua_engine = nullptr;
	}

	return lua_engine;
}

void lua_engine_set_list_entry(LuaEngine* lua_engine, SLIST_ENTRY* list_entry)
{
	assert(lua_engine != nullptr);

	if (lua_engine)
	{
		lua_engine->list_entry = list_entry;
	}
}

SLIST_ENTRY* lua_engine_get_list_entry(LuaEngine* lua_engine)
{
	assert(lua_engine != nullptr);

	return lua_engine ? lua_engine->list_entry : nullptr;
}