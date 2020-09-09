#include "shared.h"

#define ResponseMetatable "HttpResponse"

static ResponseLua* 
lua_response_check_type(lua_State* L, int index)
{
    lua_stack_guard(L, 0);

    luaL_checktype(L, index, LUA_TUSERDATA);

    ResponseLua* response_lua = (ResponseLua*)luaL_checkudata(L, index, ResponseMetatable);

    if (!response_lua)
        luaL_typerror(L, index, ResponseMetatable);

    if (!response_lua->http_context)
        luaL_error(L, "context object is invalid");

    if (!response_lua->http_response)
        luaL_error(L, "response object is invalid");

    return response_lua;
}

static int
lua_response_write(lua_State* L)
{
    lua_stack_guard(L, 0);

    ResponseLua* response_lua = lua_response_check_type(L, 1);

    luaL_checktype(L, 2, LUA_TSTRING);

    //////////////////////////////////////////////

    size_t buffer_size;
    const char* buffer = lua_tolstring(L, 2, &buffer_size);

    //////////////////////////////////////////////

    const size_t MAX_BYTES = 65535;
    size_t buffer_offset = 0;
    size_t bytes_to_write = min(max(buffer_size - buffer_offset, 0), MAX_BYTES);
    bool has_more_data = buffer_size - bytes_to_write > 0;

    IHttpResponse* http_response = response_lua->http_response;

    //////////////////////////////////////////////

    // body: string
    do
    {
        HTTP_DATA_CHUNK data_chunk = HTTP_DATA_CHUNK();
        DWORD cb_sent = 0;

        data_chunk.DataChunkType = HttpDataChunkFromMemory;
        data_chunk.FromMemory.pBuffer = PVOID((unsigned char*)buffer + buffer_offset);
        data_chunk.FromMemory.BufferLength = USHORT(bytes_to_write);

        auto hr = http_response->WriteEntityChunks(&data_chunk, 1, FALSE, has_more_data, &cb_sent);

        if (FAILED(hr)) 
        {
            return luaL_error(L, "failed to write entity chunks");
        }

        ////////////////////////////////////////

        buffer_offset += bytes_to_write;
        bytes_to_write = min(max(buffer_size - buffer_offset, 0), MAX_BYTES);
        has_more_data = buffer_size - buffer_offset > 0;

    } while (has_more_data);

    // contentType: string {optional}
    if (lua_gettop(L) >= 3 && lua_isstring(L, 3))
    {
        size_t mimetype_length;
        const char* mimetype = lua_tolstring(L, 3, &mimetype_length);

        http_response->SetHeader(
            HttpHeaderContentType, 
            mimetype, 
            (USHORT)mimetype_length, 
            TRUE
        );
    }
    else
    {
        http_response->SetHeader(
            HttpHeaderContentType, 
            "text/html", 
            (USHORT)strlen("text/html"), 
            TRUE
        );
    }

    // contentEncoding: string {optional}
    if (lua_gettop(L) >= 4 && lua_isstring(L, 4))
    {
        size_t content_encoding_length;
        const char* content_encoding = lua_tolstring(L, 4, &content_encoding_length);

        http_response->SetHeader(
            HttpHeaderContentEncoding,
            content_encoding,
            (USHORT)content_encoding_length,
            TRUE
        );
    }

    return 0;
}

static int
lua_response_clear(lua_State* L)
{
    lua_stack_guard(L, 0);

    ResponseLua* response_lua = lua_response_check_type(L, 1);

    response_lua->http_response->Clear();

    return 0;
}

static int
lua_response_clear_headers(lua_State* L)
{
    lua_stack_guard(L, 0);

    ResponseLua* response_lua = lua_response_check_type(L, 1);

    response_lua->http_response->ClearHeaders();

    return 0;
}

static int
lua_response_close_connection(lua_State* L)
{
    lua_stack_guard(L, 0);

    ResponseLua* response_lua = lua_response_check_type(L, 1);

    response_lua->http_response->CloseConnection();

    return 0;
}

static int
lua_response_disable_buffering(lua_State* L)
{
    lua_stack_guard(L, 0);

    ResponseLua* response_lua = lua_response_check_type(L, 1);

    response_lua->http_response->DisableBuffering();

    return 0;
}

static int
lua_response_set_need_disconnect(lua_State* L)
{
    lua_stack_guard(L, 0);

    ResponseLua* response_lua = lua_response_check_type(L, 1);

    response_lua->http_response->SetNeedDisconnect();

    return 0;
}

static int
lua_response_get_kernel_cache_enabled(lua_State* L)
{
    lua_stack_guard(L, 1);

    ResponseLua* response_lua = lua_response_check_type(L, 1);

    lua_pushboolean(L, response_lua->http_response->GetKernelCacheEnabled());

    return 1;
}

static int
lua_response_disable_kernel_cache(lua_State* L)
{
    lua_stack_guard(L, 0);

    ResponseLua* response_lua = lua_response_check_type(L, 1);

    response_lua->http_response->DisableKernelCache();

    return 0;
}

static int
lua_response_reset_connection(lua_State* L)
{
    lua_stack_guard(L, 0);

    ResponseLua* response_lua = lua_response_check_type(L, 1);

    response_lua->http_response->ResetConnection();

    return 0;
}

static int
lua_response_get_status(lua_State* L)
{
    lua_stack_guard(L, 1);

    ResponseLua* response_lua = lua_response_check_type(L, 1);
    lua_pushnumber(L, response_lua->http_response->GetRawHttpResponse()->StatusCode);

    return 1;
}

static int
lua_response_set_status(lua_State* L)
{
    lua_stack_guard(L, 0);

    ResponseLua* response_lua = lua_response_check_type(L, 1);

    // statusCode: number
    int status_code = (int)luaL_checkinteger(L, 2);

    // statusMessage: string
    const char* status_message = luaL_checkstring(L, 3);

    HRESULT hr = response_lua->http_response->SetStatus(status_code, status_message);

    if (FAILED(hr))
    {
        return luaL_error(L, "unable to set status, hresult: 0x%X", hr);
    }

    return 0;
}

static int
lua_response_redirect(lua_State* L)
{
    lua_stack_guard(L, 0);

    ResponseLua* response_lua = lua_response_check_type(L, 1);

    int args = lua_gettop(L);
    if (args < 4 || !lua_isstring(L, 2) || !lua_isboolean(L, 3) || !lua_isboolean(L, 4))
    {
        return luaL_error(L, "invalid number of parameters or invalid parameters");
    }

    const char* url = lua_tostring(L, 2);
    bool reset_status_code = lua_toboolean(L, 3);
    bool include_parameters = lua_toboolean(L, 4);

    HRESULT hr = response_lua->http_response->Redirect(
        url,
        reset_status_code,
        include_parameters
    );

    if (FAILED(hr))
    {
        return luaL_error(L, "unable to redirect, hresult: 0x%X", hr);
    }

    return 0;
}

static int
lua_response_set_error_desc(lua_State* L)
{
    lua_stack_guard(L, 0);

    ResponseLua* response_lua = lua_response_check_type(L, 1);

    int args = lua_gettop(L);
    if (args < 3 || !lua_isstring(L, 2) || !lua_isboolean(L, 3))
    {
        return luaL_error(L, "invalid number of parameters or invalid parameters");
    }

    size_t desc_len;
    const char* desc = lua_tolstring(L, 2, &desc_len);

    bool should_html_encode = lua_toboolean(L, 3);

    size_t desc_wide_len;
    wchar_t desc_wide[2048];
    errno_t result = mbstowcs_s(
        &desc_wide_len,
        desc_wide,
        desc,
        desc_len
    );

    if (result)
    {
        return luaL_error(L, "unable to convert to narrow string");
    }
  
    HRESULT hr = response_lua->http_response->SetErrorDescription(
        desc_wide,
        (DWORD)desc_wide_len,
        should_html_encode
    );

    if (FAILED(hr))
    {
        return luaL_error(L, "unable to set status, hresult: 0x%X", hr);
    }

    return 0;
}


static int
lua_response_set_header(lua_State* L)
{
    lua_stack_guard(L, 0);

    ResponseLua* response_lua = lua_response_check_type(L, 1);
    const char* header_name = luaL_checkstring(L, 2);

    size_t header_value_len;
    const char* header_value = luaL_checklstring(L, 3, &header_value_len);

    bool should_replace = true;

    int args = lua_gettop(L);
    if (args >= 4 && lua_isboolean(L, 4))
    {
        should_replace = lua_toboolean(L, 4);
    }

    HRESULT hr = response_lua->http_response->SetHeader(
        header_name,
        header_value,
        (USHORT)header_value_len,
        should_replace
    );

    if (FAILED(hr))
    {
        return luaL_error(L, "failed to set header, hresult: 0x%X", hr);
    }

    return 0;
}

static int
lua_response_get_header(lua_State* L)
{
    lua_stack_guard(L, 1);

    ResponseLua* response_lua = lua_response_check_type(L, 1);
    const char* header_name = luaL_checkstring(L, 2);

    USHORT header_value_size = 0;
    PCSTR header_value = response_lua->http_response->GetHeader(header_name, &header_value_size);

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
lua_response_delete_header(lua_State* L)
{
    lua_stack_guard(L, 0);

    ResponseLua* response_lua = lua_response_check_type(L, 1);
    const char* header_name = luaL_checkstring(L, 2);

    HRESULT hr = response_lua->http_response->DeleteHeader(header_name);

    if (FAILED(hr))
    {
        return luaL_error(L, "failed to delete header, hresult: 0x%X", hr);
    }

    return 0;
}

static int
lua_response_read(lua_State* L)
{
    lua_stack_guard(L, 1);

    ResponseLua* response_lua = lua_response_check_type(L, 1);
    HTTP_RESPONSE* raw_response = response_lua->http_response->GetRawHttpResponse();

    ////////////////////////////////////////////////

    auto chunk_count = raw_response->EntityChunkCount;

    if (!chunk_count) 
    {
        lua_pushnil(L);
        return 1;
    }

    ////////////////////////////////////////////////

    luaL_Buffer b;
    luaL_buffinit(L, &b);
   
    for (unsigned int i = 0; i < chunk_count; i++)
    {
        auto chunk = raw_response->pEntityChunks[i];

        if (chunk.DataChunkType == HttpDataChunkFromMemory && chunk.FromMemory.BufferLength)
        {
            luaL_addlstring(
                &b,
                (char*)chunk.FromMemory.pBuffer,
                chunk.FromMemory.BufferLength
            );
        }
    }

    luaL_pushresult(&b);

    return 1;
}

const luaL_Reg lua_response_methods[] = {

    {"Read", lua_response_read},
    {"Write", lua_response_write},
    {"Clear", lua_response_clear},
    {"ClearHeaders", lua_response_clear_headers},
    {"CloseConnection", lua_response_close_connection},
    {"DisableBuffering", lua_response_disable_buffering},
    {"SetNeedDisconnect", lua_response_set_need_disconnect},
    {"GetKernelCacheEnabled", lua_response_get_kernel_cache_enabled},
    {"DisableKernelCache", lua_response_disable_kernel_cache},

    {"ResetConnection", lua_response_reset_connection},
    {"GetStatus", lua_response_get_status},
    {"SetStatus", lua_response_set_status},
    {"Redirect", lua_response_redirect},
    {"SetErrorDescription", lua_response_set_error_desc},

    {"SetHeader", lua_response_set_header},
    {"GetHeader", lua_response_get_header},
    {"DeleteHeader", lua_response_delete_header},

    {0, 0}
};

static int 
lua_response_tostring(lua_State* L)
{
    lua_stack_guard(L, 1);

    ResponseLua* response_lua = lua_response_check_type(L, 1);

    lua_pushfstring(L, "HttpResponse: %p", response_lua);

    return 1;
}

const luaL_Reg lua_response_meta[] =
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
        lua_stack_guard(L, 0);

        luaL_openlib(L, ResponseMetatable, lua_response_methods, 0);
        luaL_newmetatable(L, ResponseMetatable);

        luaL_openlib(L, 0, lua_response_meta, 0);
        lua_pushliteral(L, "__index");
        lua_pushvalue(L, -3);
        lua_rawset(L, -3);

        lua_pushliteral(L, "__metatable");
        lua_pushvalue(L, -3);
        lua_rawset(L, -3);

        lua_pop(L, 2);
    }
}

ResponseLua* 
lua_response_push(lua_State* L)
{
    ResponseLua* response_lua = nullptr;

    if (L)
    {
        lua_stack_guard(L, 1);

        response_lua = (ResponseLua*)lua_newuserdata(L, sizeof(ResponseLua));
        luaL_getmetatable(L, ResponseMetatable);
        lua_setmetatable(L, -2);
    }

    return response_lua;
}