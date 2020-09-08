#pragma once
#include "http_module.h"

class ModuleFactory : public IHttpModuleFactory
{
public:
	ModuleFactory(IHttpServer* http_server) 
	{
		m_lua_state_manager = lua_state_manager_create(http_server);

		if (!m_lua_state_manager)
			throw std::exception("lua state manager is invalid");
	};

	virtual HRESULT GetHttpModule(OUT CHttpModule ** ppModule, IN IModuleAllocator * pAllocator)
	{
		// Set our unreferenced param...
		UNREFERENCED_PARAMETER(pAllocator);
		 
		// Create a new instance.
		HttpModule* pModule = new (std::nothrow) HttpModule(m_lua_state_manager);

		// Test for an error.
		if (!pModule)
		{
			// Return an error if the factory cannot create th e instance.
			return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
		}
		else
		{
			// Return a pointer to the module.
			*ppModule = pModule;
			pModule = nullptr;

			// Return a success status.
			return S_OK;
		}
	}

	virtual void Terminate()
	{
		m_lua_state_manager = lua_state_manager_destroy(m_lua_state_manager);

		// Remove the class from memory.
		delete this;
	}

private:
	LuaStateManager* m_lua_state_manager;
};

