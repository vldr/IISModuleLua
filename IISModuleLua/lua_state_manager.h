#pragma once
#include "shared.h"

#ifndef _LUA_STATE_MANAGER
#define _LUA_STATE_MANAGER

#define LUA_STATE_MAX_POOL_SIZE 32

typedef struct _LuaEngine LuaEngine;
typedef struct _LuaStateManager LuaStateManager;

LuaStateManager* lua_state_manager_create(void);
LuaStateManager* lua_state_manager_destroy(LuaStateManager* lsm);
LuaEngine* lua_state_manager_aquire(LuaStateManager* lsm);
LuaEngine* lua_state_manager_release(LuaStateManager* lsm, LuaEngine* lua_engine);

#endif