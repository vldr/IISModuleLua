#pragma once
#include "shared.h"

#ifndef _LUA_ENGINE
#define _LUA_ENGINE

typedef struct _LuaEngine LuaEngine;

int lua_engine_printf(const char* format, ...);
LuaEngine* lua_engine_create(const wchar_t* name);
LuaEngine* lua_engine_destroy(LuaEngine* lua_engine);
REQUEST_NOTIFICATION_STATUS lua_engine_begin_request(
    LuaEngine* lua_engine, 
    IHttpContext* http_context
);

#endif