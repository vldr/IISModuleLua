#pragma once
#include "http_module.h"

class ModuleFactory : public IHttpModuleFactory
{
public:
	ModuleFactory(IHttpServer* http_server) 
		: m_http_server(http_server), m_lua_state_manager(lua_state_manager_start()) {};

	virtual HRESULT GetHttpModule(OUT CHttpModule ** ppModule, IN IModuleAllocator * pAllocator)
	{
		// Set our unreferenced param...
		UNREFERENCED_PARAMETER(pAllocator);
		 
		// Create a new instance.
		HttpModule* pModule = HttpModule::create();

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
			pModule = NULL;
			// Return a success status.
			return S_OK;
		}
	}

	virtual void Terminate()
	{
		// Remove the class from memory.
		delete this;
	}

private:
	IHttpServer* m_http_server;
	LuaStateManager* m_lua_state_manager;
};

