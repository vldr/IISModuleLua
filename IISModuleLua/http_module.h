#pragma once
#include "shared.h"

class HttpModule : public CHttpModule
{
public:
	REQUEST_NOTIFICATION_STATUS OnBeginRequest(
		IN IHttpContext* pHttpContext,
		IN IHttpEventProvider* pProvider
	);

	HttpModule(LuaStateManager* lua_state_manager) 
		: m_lua_state_manager(lua_state_manager), m_lua_engine(nullptr)
	{};

	~HttpModule() 
	{
		m_lua_engine = lua_state_manager_release(m_lua_state_manager, m_lua_engine);
	};

private:
	LuaEngine* m_lua_engine = nullptr;
	LuaStateManager* m_lua_state_manager = nullptr;
};
