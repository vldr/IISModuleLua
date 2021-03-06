#pragma once

#define _WINSOCKAPI_
#include <windows.h>
#include <sal.h>
#include <ws2tcpip.h>
#include <httpserv.h>
#include <stdio.h>
#include <assert.h>
#include <Shlobj.h>
#include <exception>
#include <ws2tcpip.h>
#include <atomic>
#include <stdexcept>

extern "C"
{
#include "LuaJit/lua.h"
#include "LuaJit/lualib.h"
#include "LuaJit/lauxlib.h"
}  

#ifdef _WIN64
#ifdef _DEBUG
#pragma comment(lib, "LuaJit/lua51.lib")
#else
#pragma comment(lib, "LuaJit/lua51.release.lib")
#endif
#endif

#pragma comment(lib, "Ws2_32.lib")

#include "lua_engine.h"
#include "lua_response.h"
#include "lua_request.h"
#include "lua_state_manager.h"
#include "lua_stack_guard.h"