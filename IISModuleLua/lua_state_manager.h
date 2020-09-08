#pragma once
#include "shared.h"

#ifndef _LUA_STATE_MANAGER
#define _LUA_STATE_MANAGER

typedef struct _LuaEngine LuaEngine;
typedef struct _LuaStateManager LuaStateManager;

LuaStateManager* lua_state_manager_create(IHttpServer* http_server);
LuaStateManager* lua_state_manager_destroy(LuaStateManager* lsm);
LuaEngine* lua_state_manager_aquire(LuaStateManager* lsm);
LuaEngine* lua_state_manager_release(LuaStateManager* lsm, LuaEngine* lua_engine);

#endif