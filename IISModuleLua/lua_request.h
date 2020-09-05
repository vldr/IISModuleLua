#pragma once
#include "shared.h"

#ifndef _LUA_REQUEST
#define _LUA_REQUEST

typedef struct _RequestLua
{
    IHttpRequest* http_request;
} RequestLua;

void lua_request_register(lua_State* L);
RequestLua* lua_request_push(lua_State* L);

#endif