#include "shared.h"

#define ResponseMetatable "HttpResponse"

static ResponseLua* 
lua_response_check_type(lua_State* L, int index)
{
    luaL_checktype(L, index, LUA_TUSERDATA);

    ResponseLua* response_lua = (ResponseLua*)luaL_checkudata(L, index, ResponseMetatable);

    if (!response_lua)
        luaL_typerror(L, index, ResponseMetatable);

    if (!response_lua->http_context)
        luaL_error(L, "context object is invalid");

    if (!response_lua->http_context->GetResponse())
        luaL_error(L, "response object is invalid");

    return response_lua;
}

static int 
lua_response_status(lua_State* L)
{
    ResponseLua* response_lua = lua_response_check_type(L, 1);

    lua_pushnumber(L, response_lua->http_context->GetResponse()->GetRawHttpResponse()->StatusCode);

    return 1;
}

const luaL_reg lua_response_methods[] = {

    {"status", lua_response_status},
    {0, 0}
};

static int 
lua_response_tostring(lua_State* L)
{
    ResponseLua* response_lua = lua_response_check_type(L, 1);

    lua_pushfstring(L, "HttpResponse: %p", response_lua);

    return 1;
}

const luaL_reg lua_response_meta[] =
{
    {"__tostring", lua_response_tostring},
    {0, 0}
};

void 
lua_response_register(lua_State* L)
{
    assert(L != nullptr);

    if (L)
    {
        luaL_openlib(L, ResponseMetatable, lua_response_methods, 0);
        luaL_newmetatable(L, ResponseMetatable);

        luaL_openlib(L, 0, lua_response_meta, 0);
        lua_pushliteral(L, "__index");
        lua_pushvalue(L, -3);
        lua_rawset(L, -3);

        lua_pushliteral(L, "__metatable");
        lua_pushvalue(L, -3);
        lua_rawset(L, -3);

        lua_pop(L, 1);
    }
}

ResponseLua* 
lua_response_push(lua_State* L)
{
    ResponseLua* response_lua = nullptr;

    if (L)
    {
        response_lua = (ResponseLua*)lua_newuserdata(L, sizeof(ResponseLua));
        luaL_getmetatable(L, ResponseMetatable);
        lua_setmetatable(L, -2);
    }

    return response_lua;
}