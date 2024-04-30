module;
#include "hv/HttpServer.h"
export module ApiManager;

import :ApiLogin;

using namespace hv;

export void ApiInit(HttpService* service)
{
	ApiLogin(service);
}