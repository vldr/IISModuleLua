#pragma once
#include "shared.h"

#ifndef _LUA_ENGINE
#define _LUA_ENGINE

lua_State* lua_engine_create(void);
void lua_engine_destroy(lua_State* L);
void lua_engine_call_begin_request(lua_State* L, IHttpResponse* http_response, IHttpRequest* http_request);

#endif