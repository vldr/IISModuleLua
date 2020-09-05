#pragma once
#include "shared.h"

#ifndef _LUA_ENGINE
#define _LUA_ENGINE

typedef struct _LuaEngine
{
    lua_State* L;
    HANDLE mutex_handle;
} LuaEngine;

LuaEngine* lua_engine_create(void);
void lua_engine_destroy(LuaEngine* lua_engine);
void lua_engine_call_begin_request(LuaEngine* lua_engine, IHttpResponse* http_response, IHttpRequest* http_request);

#endif