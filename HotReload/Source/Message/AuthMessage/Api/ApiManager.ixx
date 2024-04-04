module;
#include "hv/HThread.h"
#include "hv/HttpMessage.h"
#include "hv/HttpService.h"

export module ApiManager;

import :ApiLogin;

using namespace hv;

export void ApiInit(HttpService* service)
{
	ApiLogin(service);
}