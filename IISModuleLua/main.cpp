#include "module_factory.h"

/**
 * RegisterModule
 */
HRESULT WINAPI RegisterModule(DWORD dwServerVersion, IHttpModuleRegistrationInfo* pModuleInfo, IHttpServer* pHttpServer)
{ 
	UNREFERENCED_PARAMETER(dwServerVersion); 

	return pModuleInfo->SetRequestNotifications(new ModuleFactory(pHttpServer), RQ_BEGIN_REQUEST, 0);
} 