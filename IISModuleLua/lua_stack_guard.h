#pragma once
#include "shared.h"

#ifndef _LUA_STACK_GUARD
#define _LUA_STACK_GUARD

class LuaStackGuard
{
public:
	LuaStackGuard(lua_State* L, int expected_return_values)
		: L(L), m_stack_entry_count(lua_gettop(L) + expected_return_values) {}

	~LuaStackGuard()
	{
		if (m_stack_entry_count != lua_gettop(L))
		{
			lua_engine_printf("lua stack guard fired, difference %d, before %d, after %d\n", 
				lua_gettop(L) - m_stack_entry_count, 
				m_stack_entry_count, 
				lua_gettop(L)
			);

			DebugBreak();
		}
	}
private:
	int m_stack_entry_count;
	lua_State* L;
};

#ifdef _DEBUG
#define lua_stack_guard(L, n) LuaStackGuard lua_stack_guard(L, n);
#else
#define lua_stack_guard(L, n)
#endif

#endif