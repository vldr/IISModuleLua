#pragma once
#include "shared.h"

#ifndef _LUA_ENGINE_
#define _LUA_ENGINE_

#define lua_engine_load_file(L, s) (luaL_loadfile(L, s) || lua_pcall(L, 0, 0, 0))

typedef struct _LuaEngine
{
	lua_State* L;
	HANDLE mutex_handle;
	HANDLE directory_changes_handle;
	char file_path[MAX_PATH];

	SLIST_ENTRY* list_entry;
} LuaEngine;

int lua_engine_printf(const char* format, ...);
LuaEngine* lua_engine_create(const wchar_t* name);
LuaEngine* lua_engine_destroy(LuaEngine* lua_engine);
REQUEST_NOTIFICATION_STATUS lua_engine_begin_request(
    LuaEngine* lua_engine, 
    IHttpContext* http_context
);

#endif
