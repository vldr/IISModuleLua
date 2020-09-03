#pragma once
#include "shared.h"

#ifndef _LUA_HTTP_CONTEXT
#define _LUA_HTTP_CONTEXT

typedef struct _ResponseLua
{
    IHttpResponse* http_response;
} ResponseLua;

void response_register(lua_State* L);
ResponseLua* response_push(lua_State* L);

#endif