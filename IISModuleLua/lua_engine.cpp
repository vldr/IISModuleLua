#include "lua_engine.h"

int begin_request_function_reference = LUA_NOREF;

static int lua_engine_printf(const char* format, ...)
{
	char str[1024];

	va_list argptr;
	va_start(argptr, format);
	int ret = vsnprintf(str, sizeof(str), format, argptr);
	va_end(argptr);

	OutputDebugStringA(str);

	return ret;
}

static int lua_engine_register(lua_State* L) 
{
	assert(L != nullptr);

	luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_pushvalue(L, 1);

	int ref = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_remove(L, -1);

	assert(ref != LUA_NOREF);
	assert(ref != LUA_REFNIL);

	if (ref == LUA_NOREF || ref == LUA_REFNIL)
	{
		return luaL_error(L, "failed to retrieve a reference to the provided function");
	}

	if (begin_request_function_reference != LUA_NOREF) 
	{
		luaL_unref(L, LUA_REGISTRYINDEX, begin_request_function_reference);
	}

	begin_request_function_reference = ref;

	return 0;
}

static int lua_engine_print(lua_State* L)
{
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

	return 0;
}

static int lua_engine_http_newindex(lua_State* L)
{
	return luaL_error(L, "attempt to update a read-only table");
}

static bool lua_engine_lock(LuaEngine* lua_engine)
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

static bool lua_engine_unlock(LuaEngine* lua_engine)
{	
	assert(lua_engine != nullptr);
	assert(lua_engine->mutex_handle != nullptr);

	return !ReleaseMutex(lua_engine->mutex_handle);
}

int lua_engine_call_begin_request(LuaEngine* lua_engine, IHttpResponse* http_response, IHttpRequest* http_request)
{
	assert(lua_engine != nullptr);
	assert(http_response != nullptr);
	assert(http_request != nullptr);

	if (lua_engine == nullptr || http_response == nullptr || http_request == nullptr && lua_engine_lock(lua_engine))
		return;

	lua_State* L = lua_engine->L;

	assert(L != nullptr);

	int result = RQ_NOTIFICATION_CONTINUE;

	if (L && begin_request_function_reference != LUA_NOREF)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, begin_request_function_reference);

		if (lua_isfunction(L, -1))
		{
			ResponseLua* response_lua = lua_response_push(L);
			RequestLua* request_lua = lua_request_push(L);

			response_lua->http_response = http_response;
			request_lua->http_request = http_request;

			if (lua_pcall(L, 2, 1, 0) == 0)
			{
				result = (int)lua_tonumber(L, -1);
				lua_engine_printf("[C] lua_engine_call_begin_request called: %d\n", result);
			}
			else
			{
				lua_engine_printf("%s\n", lua_tostring(L, -1));
			}

			response_lua->http_response = nullptr;
			request_lua->http_request = nullptr;
		}

		lua_pop(L, 1);
	}

	lua_engine_unlock(lua_engine);

	return result ? RQ_NOTIFICATION_FINISH_REQUEST : RQ_NOTIFICATION_CONTINUE;
}

void lua_engine_setup_http_object(lua_State* L)
{
	assert(L != nullptr);

	struct constant_pair { const char* key; int val; };
	struct constant_pair constants[] =
	{
		{ "Continue", 1 },
		{ "Finish", 2 },
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

LuaEngine* lua_engine_create(void)
{
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

	lua_engine = (LuaEngine*)malloc(sizeof(*lua_engine));

	assert(lua_engine != nullptr);

	if (!lua_engine)
	{
		lua_engine_printf("failed to create lua engine object\n");
		goto error;
	}

	//////////////////////////////////////////

	L = luaL_newstate();

	assert(L != nullptr);

	if (!L)
	{
		lua_engine_printf("failed to create lua state\n");
		goto error;
	}

	// Setup libraries.
	luaL_openlibs(L);

	// Setup constants.
	lua_engine_setup_http_object(L);

	// Register ResponseLua, and register RequestLua.
	lua_response_register(L);
	lua_request_register(L);

	// Register global functions.
	lua_register(L, "print", lua_engine_print);
	lua_register(L, "register", lua_engine_register);

	if (luaL_dofile(L, "C:\\Users\\vlad\\Documents\\Lua\\test.lua") != 0)
	{
		lua_engine_printf("error: %s\n", lua_tostring(L, -1));
	}

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

LuaEngine* lua_engine_destroy(LuaEngine* lua_engine)
{
	assert(lua_engine != nullptr);
	assert(lua_engine->L != nullptr);
	assert(lua_engine->mutex_handle != nullptr);

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

		free(lua_engine);
		lua_engine = nullptr;
	}

	begin_request_function_reference = LUA_NOREF;

	return lua_engine;
}