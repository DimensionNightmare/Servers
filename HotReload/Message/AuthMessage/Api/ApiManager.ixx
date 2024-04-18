module;
export module ApiManager;

import :ApiLogin;

export void ApiInit(HttpService* service)
{
	ApiLogin(service);
}