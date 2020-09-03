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

	OutputDebugStringA("called!\n");

	begin_request_function_reference = ref;
	return 0;
}

void lua_engine_call_begin_request(lua_State* L, IHttpResponse* http_response, IHttpRequest* http_request)
{
	assert(L != nullptr);
	assert(http_response != nullptr);
	assert(http_request != nullptr);
	assert(begin_request_function_reference != LUA_NOREF);

	if (L == nullptr || http_response == nullptr || http_request == nullptr)
		return;

	if (begin_request_function_reference != LUA_NOREF)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, begin_request_function_reference);

		if (lua_isfunction(L, -1))
		{
			ResponseLua* response_lua = response_push(L);
			response_lua->http_response = http_response;

			if (lua_pcall(L, 1, 1, 0) == 0)
			{
				lua_engine_printf("[C] lua_engine_call_begin_request called: %d", (int)lua_tonumber(L, -1));
			}
			else
			{
				lua_engine_printf("pcall failed: %s\n", lua_tostring(L, -1));
			}

			response_lua->http_response = nullptr;
		}
		else
		{
			lua_engine_printf("a function type was not registered\n");
		}

		lua_pop(L, 1);
	}
}

static int lua_engine_dprint(lua_State* L)
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

lua_State* lua_engine_create(void)
{
	lua_State* L = luaL_newstate();

	assert(L != nullptr);

	if (L)
	{
		// Setup libraries.
		luaL_openlibs(L);

		// Register ResponseLua.
		response_register(L);

		// Setup 'dprint' function.
		lua_register(L, "dprint", lua_engine_dprint);

		// Setup 'register' function.
		lua_register(L, "register", lua_engine_register);

		if (luaL_dofile(L, "C:\\Users\\vlad\\Documents\\Lua\\test.lua") == 0)
		{

		}
		else
		{
			OutputDebugStringA("failed to execute file.");
		}
	}

	return L;
}

void lua_engine_destroy(lua_State* L)
{
	assert(L != nullptr);

	lua_close(L);
}