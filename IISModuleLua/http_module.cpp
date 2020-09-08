#include "http_module.h"

/**
 * OnBeginRequest 
 */
REQUEST_NOTIFICATION_STATUS HttpModule::OnBeginRequest(
	IN IHttpContext* pHttpContext, 
	IN IHttpEventProvider* pProvider
)
{
	UNREFERENCED_PARAMETER(pProvider);

	if (!pHttpContext || !pHttpContext->GetResponse() || !pHttpContext->GetRequest())
		return RQ_NOTIFICATION_CONTINUE;

	if (!m_lua_engine)
		m_lua_engine = lua_state_manager_aquire(m_lua_state_manager);
	
	return lua_engine_begin_request(
		m_lua_engine, 
		pHttpContext
	);
}  