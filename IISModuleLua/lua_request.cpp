#include "shared.h"

#define RequestMetatable "HttpRequest"

static RequestLua* 
lua_request_check_type(lua_State* L, int index)
{
    luaL_checktype(L, index, LUA_TUSERDATA);

    RequestLua* request_lua = (RequestLua*)luaL_checkudata(L, index, RequestMetatable);

    if (!request_lua)
        luaL_typerror(L, index, RequestMetatable);

    if (!request_lua->http_request)
        luaL_error(L, "object is invalid");

    return request_lua;
}

static int 
lua_request_get_full_url(lua_State* L)
{
    RequestLua* request_lua = lua_request_check_type(L, 1);
    HTTP_REQUEST* raw_request = request_lua->http_request->GetRawHttpRequest();

    char converted[1024];

    sprintf_s(
        converted, 
        "%.*ls", 
        raw_request->CookedUrl.FullUrlLength / sizeof(wchar_t),
        raw_request->CookedUrl.pFullUrl   
    );

    lua_pushstring(L, converted);
  
    return 1;
}

const luaL_reg lua_request_methods[] = {

    {"getFullUrl", lua_request_get_full_url},
    {0, 0}
};

int 
lua_request_tostring(lua_State* L)
{
    RequestLua* request_lua = lua_request_check_type(L, 1);

    lua_pushfstring(L, "%s: %p", RequestMetatable, request_lua);

    return 1;
}

const luaL_reg lua_request_meta[] =
{
    {"__tostring", lua_request_tostring},
    {0, 0}
};

void 
lua_request_register(lua_State* L)
{
    luaL_openlib(L, RequestMetatable, lua_request_methods, 0);
    luaL_newmetatable(L, RequestMetatable);

    luaL_openlib(L, 0, lua_request_meta, 0);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);

    lua_pushliteral(L, "__metatable");
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);

    lua_pop(L, 1);
}

RequestLua* 
lua_request_push(lua_State* L)
{
    RequestLua* request_lua = (RequestLua*)lua_newuserdata(L, sizeof RequestLua);
    luaL_getmetatable(L, RequestMetatable);
    lua_setmetatable(L, -2);

    return request_lua;
}