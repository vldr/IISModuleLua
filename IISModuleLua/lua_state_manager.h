#pragma once
#include "shared.h"

#ifndef _LUA_STATE_MANAGER
#define _LUA_STATE_MANAGER

#define LUA_STATE_MAX_POOL_SIZE 32

typedef struct _LuaStateManager LuaStateManager;

LuaStateManager* lua_state_manager_start(void);
lua_State* lua_state_manager_aquire(LuaStateManager* lsm);
bool lua_state_manager_release(LuaStateManager* lsm, lua_State* L);

#endif