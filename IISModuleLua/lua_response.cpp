#include "shared.h"

#define ResponseMetatable "HttpResponse"

static ResponseLua* response_check_type(lua_State* L, int index)
{
    luaL_checktype(L, index, LUA_TUSERDATA);

    ResponseLua* response_lua = (ResponseLua*)luaL_checkudata(L, index, ResponseMetatable);

    if (!response_lua)
        luaL_typerror(L, index, ResponseMetatable);

    if (!response_lua->http_response)
        luaL_error(L, "object is invalid");

    return response_lua;
}

int response_status(lua_State* L)
{
    ResponseLua* response_lua = response_check_type(L, 1);

    lua_pushnumber(
        L, 
        response_lua->http_response->GetRawHttpResponse()->StatusCode
    );

    return 1;
}

// Methods
const luaL_reg response_methods[] = {

    {"status", response_status},
    {0, 0}
};

// Metatable
const luaL_reg response_meta[] = 
{
    {0, 0}
};

void response_register(lua_State* L)
{
    luaL_openlib(L, ResponseMetatable, response_methods, 0);
    luaL_newmetatable(L, ResponseMetatable);

    luaL_openlib(L, 0, response_meta, 0);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);

    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);

    lua_pop(L, 1);
}

ResponseLua* response_push(lua_State* L)
{
    ResponseLua* response_lua = (ResponseLua*)lua_newuserdata(L, sizeof ResponseLua);
    luaL_getmetatable(L, ResponseMetatable);
    lua_setmetatable(L, -2);

    return response_lua;
}