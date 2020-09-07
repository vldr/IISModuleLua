#include "shared.h"

#define RequestMetatable "HttpRequest"

static RequestLua* 
lua_request_check_type(lua_State* L, int index)
{
    luaL_checktype(L, index, LUA_TUSERDATA);

    RequestLua* request_lua = (RequestLua*)luaL_checkudata(L, index, RequestMetatable);

    if (!request_lua)
        luaL_typerror(L, index, RequestMetatable);

    if (!request_lua->http_context)
        luaL_error(L, "context object is invalid");
    
    if (!request_lua->http_context->GetRequest())
        luaL_error(L, "request object is invalid");

    return request_lua;
}

static int 
lua_request_get_full_url(lua_State* L)
{
    RequestLua* request_lua = lua_request_check_type(L, 1);
    HTTP_REQUEST* raw_request = request_lua->http_context->GetRequest()->GetRawHttpRequest();

    char converted[2048];

    sprintf_s(
        converted, 
        "%.*ls", 
        (int)(raw_request->CookedUrl.FullUrlLength / sizeof(wchar_t)),
        raw_request->CookedUrl.pFullUrl   
    );

    lua_pushstring(L, converted);
  
    return 1;
}

static int
lua_request_get_abs_url(lua_State* L)
{
    RequestLua* request_lua = lua_request_check_type(L, 1);
    HTTP_REQUEST* raw_request = request_lua->http_context->GetRequest()->GetRawHttpRequest();

    char converted[2048];

    sprintf_s(
        converted,
        "%.*ls",
        (int)(raw_request->CookedUrl.AbsPathLength / sizeof(wchar_t)),
        raw_request->CookedUrl.pAbsPath
    );

    lua_pushstring(L, converted);

    return 1;
}

static int
lua_request_get_host_url(lua_State* L)
{
    RequestLua* request_lua = lua_request_check_type(L, 1);
    HTTP_REQUEST* raw_request = request_lua->http_context->GetRequest()->GetRawHttpRequest();

    char converted[2048];

    sprintf_s(
        converted,
        "%.*ls",
        (int)(raw_request->CookedUrl.HostLength / sizeof(wchar_t)),
        raw_request->CookedUrl.pHost
    );

    lua_pushstring(L, converted);

    return 1;
}

static int
lua_request_get_querystring(lua_State* L)
{
    RequestLua* request_lua = lua_request_check_type(L, 1);
    HTTP_REQUEST* raw_request = request_lua->http_context->GetRequest()->GetRawHttpRequest();

    char converted[2048];

    sprintf_s(
        converted,
        "%.*ls",
        (int)(raw_request->CookedUrl.QueryStringLength / sizeof(wchar_t)),
        raw_request->CookedUrl.pQueryString
    );

    lua_pushstring(L, converted);

    return 1;
}

static int
lua_request_set_url(lua_State* L)
{
    RequestLua* request_lua = lua_request_check_type(L, 1);
    const char* url = luaL_checkstring(L, 2);

    bool reset_querystring = true;

    if (lua_gettop(L) >= 3 && lua_isboolean(L, 3))
    {
        reset_querystring = lua_toboolean(L, 3);
    }

    HRESULT hr = request_lua->http_context->GetRequest()->SetUrl(
        url, 
        (DWORD)strlen(url), 
        reset_querystring
    );

    if (FAILED(hr))
    {
        return luaL_error(L, "failed to set url");
    }

    return 0;
}

static int
lua_request_read(lua_State* L)
{
    RequestLua* request_lua = lua_request_check_type(L, 1);
    IHttpRequest* http_request = request_lua->http_context->GetRequest();

    DWORD remaining_bytes = http_request->GetRemainingEntityBytes();

    if (remaining_bytes)
    {
        luaL_Buffer b;
        luaL_buffinit(L, &b);

        while (remaining_bytes)
        {
            unsigned char buffer[4096];
            DWORD bytes_read = 0;

            HRESULT hr = http_request->ReadEntityBody(buffer, sizeof(buffer), FALSE, &bytes_read, 0);

            if (FAILED(hr)) 
            {   
                return luaL_error(L, "failed to read entity body");
            }

            if (bytes_read)
            {
                luaL_addlstring(&b, (char*)buffer, bytes_read);
            }

            remaining_bytes = http_request->GetRemainingEntityBytes();
        }

        luaL_pushresult(&b);

        // rewrite: bool {optional}
        if (lua_gettop(L) >= 2 && lua_isboolean(L, 2) && lua_toboolean(L, 2))
        {
            size_t lua_buffer_length;
            const char* lua_buffer = lua_tolstring(L, -1, &lua_buffer_length);

            void* context_buffer = request_lua->http_context->AllocateRequestMemory(
                (DWORD)lua_buffer_length
            );

            if (!context_buffer)
            {
                return luaL_error(L, "failed to allocate request memory");
            }

            memcpy(context_buffer, lua_buffer, lua_buffer_length);

            HRESULT hr = http_request->InsertEntityBody(
                context_buffer, 
                (DWORD)lua_buffer_length
            );

            if (FAILED(hr)) 
            {
                return luaL_error(L, "failed to insert entity body");
            }
        }
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

static int
lua_request_set_header(lua_State* L)
{
    RequestLua* request_lua = lua_request_check_type(L, 1);
    const char* header_name = luaL_checkstring(L, 2);
    const char* header_value = luaL_checkstring(L, 3);
    bool should_replace = true;

    int args = lua_gettop(L);
    if (args >= 4 && lua_isboolean(L, 4))
    {
        should_replace = lua_toboolean(L, 4);
    }

    HRESULT hr = request_lua->http_context->GetRequest()->SetHeader(
        header_name, 
        header_value, 
        (unsigned int)strlen(header_value), 
        should_replace
    );

    if (FAILED(hr))
    {
        return luaL_error(L, "failed to set header");
    }

    return 0;
}

static int
lua_request_get_header(lua_State* L)
{
    RequestLua* request_lua = lua_request_check_type(L, 1);
    const char* header_name = luaL_checkstring(L, 2);

    USHORT header_value_size = 0;
    PCSTR header_value = request_lua->http_context->GetRequest()->GetHeader(header_name, &header_value_size);

    if (header_value)
    {
        lua_pushlstring(L, header_value, header_value_size);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

static int
lua_request_delete_header(lua_State* L)
{
    RequestLua* request_lua = lua_request_check_type(L, 1);
    const char* header_name = luaL_checkstring(L, 2);

    HRESULT hr = request_lua->http_context->GetRequest()->DeleteHeader(header_name);

    if (FAILED(hr))
    {
        return luaL_error(L, "failed to delete header");
    }

    return 0;
}

static bool lua_request_format_sockaddr(PSOCKADDR address, char* ip_address)
{
    assert(address != nullptr);
    assert(ip_address != nullptr);

    if (address && ip_address)
    {
        if (address->sa_family == AF_INET)
        {
            sockaddr_in* socket = (sockaddr_in*)address;

            InetNtopA(socket->sin_family, &socket->sin_addr, ip_address, sizeof ip_address);

            return true;
        }
        else if (address->sa_family == AF_INET6)
        {
            sockaddr_in6* socket = (sockaddr_in6*)address;

            InetNtopA(socket->sin6_family, &socket->sin6_addr, ip_address, sizeof ip_address);

            return true;
        }
    }

    return false;
}

static int
lua_request_get_localaddress(lua_State* L)
{
    RequestLua* request_lua = lua_request_check_type(L, 1);

    char ip_address[INET6_ADDRSTRLEN] = { 0 };
    PSOCKADDR sockaddr = request_lua->http_context->GetRequest()->GetLocalAddress();

    if (!lua_request_format_sockaddr(sockaddr, ip_address))
    {
        return luaL_error(L, "failed to get local address");
    }

    lua_pushstring(L, ip_address);

    return 1;
}

static int
lua_request_get_remoteaddress(lua_State* L)
{
    RequestLua* request_lua = lua_request_check_type(L, 1);

    char ip_address[INET6_ADDRSTRLEN] = { 0 };
    PSOCKADDR sockaddr = request_lua->http_context->GetRequest()->GetRemoteAddress();

    if (!lua_request_format_sockaddr(sockaddr, ip_address))
    {
        return luaL_error(L, "failed to get remote address");
    }

    lua_pushstring(L, ip_address);

    return 1;
}

static int
lua_request_get_method(lua_State* L)
{
    RequestLua* request_lua = lua_request_check_type(L, 1);

    PCSTR http_method = request_lua->http_context->GetRequest()->GetHttpMethod();

    if (!http_method)
    {
        return luaL_error(L, "failed to get method");
    }

    lua_pushstring(L, http_method);

    return 1;
}

const luaL_reg lua_request_methods[] = {

    {"read", lua_request_read},

    {"setUrl", lua_request_set_url},
    {"getFullUrl", lua_request_get_full_url},
    {"getAbsUrl", lua_request_get_abs_url},
    {"getHostUrl", lua_request_get_host_url},
    {"getQueryString", lua_request_get_querystring},

    {"setHeader", lua_request_set_header},
    {"getHeader", lua_request_get_header},
    {"deleteHeader", lua_request_delete_header},

    {"getMethod", lua_request_get_method},

    {"getLocalAddress", lua_request_get_localaddress},
    {"getRemoteAddress", lua_request_get_remoteaddress},   

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
    if (L)
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
}

RequestLua* 
lua_request_push(lua_State* L)
{
    RequestLua* request_lua = nullptr;

    if (L)
    {
        request_lua = (RequestLua*)lua_newuserdata(L, sizeof(RequestLua));
        luaL_getmetatable(L, RequestMetatable);
        lua_setmetatable(L, -2);
    }
  
    return request_lua;
}