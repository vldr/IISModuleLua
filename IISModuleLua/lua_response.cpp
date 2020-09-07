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

static int
lua_response_write(lua_State* L)
{
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

    IHttpResponse* http_response = response_lua->http_context->GetResponse();

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

    // contentType: String {optional}
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

    // contentEncoding: String {optional}
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

const luaL_reg lua_response_methods[] = {

    {"status", lua_response_status},
    {"write", lua_response_write},
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