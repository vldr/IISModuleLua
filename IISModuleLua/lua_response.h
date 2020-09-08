#pragma once
#include "shared.h"

#ifndef _LUA_RESPONSE
#define _LUA_RESPONSE

typedef struct _ResponseLua
{
    IHttpContext* http_context;
    IHttpResponse* http_response;
} ResponseLua;

void lua_response_register(lua_State* L);
ResponseLua* lua_response_push(lua_State* L);

#endif
