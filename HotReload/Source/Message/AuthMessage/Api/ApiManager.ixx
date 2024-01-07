module;
#include "hv/HThread.h"
#include "hv/HttpMessage.h"
#include "hv/HttpService.h"

export module ApiManager;

import :ApiLogin;

using namespace hv;

export void ApiInit(HttpService* service)
{
	service->Static("/", ".");

	service->EnableForwardProxy();

	service->AddTrustProxy("*httpbin.org");

	service->Proxy("/httpbin/", "http://httpbin.org/");

	service->AllowCORS();

	service->Use([](HttpRequest* req, HttpResponse* resp) {
		resp->SetHeader("X-Request-tid", hv::to_string(hv_gettid()));
		return HTTP_STATUS_NEXT;
	});

	ApiLogin(service);
}