#pragma once
#include "shared.h"

class HttpModule : public CHttpModule
{
public:
	REQUEST_NOTIFICATION_STATUS OnBeginRequest(
		IN IHttpContext* pHttpContext,
		IN IHttpEventProvider* pProvider
	);

	HttpModule(lua_State* L) : L(L) {};
	~HttpModule() { lua_engine_destroy(L); };

	static HttpModule* create()
	{
		lua_State* L = lua_engine_create();
		HttpModule* httpModule = nullptr;

		if (L)
		{
			httpModule = new (std::nothrow) HttpModule(L);

			if (!httpModule)
			{
				lua_engine_destroy(L);
			}
		}

		return httpModule;
	}
private:
	lua_State* L = nullptr;
};
