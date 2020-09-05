#pragma once

#define _WINSOCKAPI_
#include <windows.h>
#include <sal.h>
#include <ws2tcpip.h>
#include <httpserv.h>
#include <stdio.h>
#include <new> 
#include <assert.h>

extern "C"
{
#include "LuaJit/lua.h"
#include "LuaJit/lualib.h"
#include "LuaJit/lauxlib.h"
};

#ifdef _WIN64
#pragma comment(lib, "LuaJit/lua51.lib")
#endif

#include "lua_engine.h"
#include "lua_response.h"
#include "lua_request.h"
#include "lua_state_manager.h"