#include "http_module.h"

/**
 * OnBeginRequest 
 */
REQUEST_NOTIFICATION_STATUS HttpModule::OnBeginRequest(IN IHttpContext* pHttpContext, IN IHttpEventProvider* pProvider)
{
	if (!pHttpContext)
		return RQ_NOTIFICATION_CONTINUE;

	lua_engine_call_begin_request(this->L, pHttpContext->GetResponse(), pHttpContext->GetRequest());

	return RQ_NOTIFICATION_FINISH_REQUEST;
}  