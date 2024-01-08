module;
#include "hv/HThread.h"
#include "hv/HttpMessage.h"
#include "hv/HttpService.h"
#include "hv/hurl.h"

export module ApiManager:ApiLogin;

using namespace std;
using namespace hv;

export void ApiLogin(HttpService* service)
{
	service->POST("/Login/Auth", [](const HttpContextPtr& ctx) {
		return ctx->send(ctx->body(), ctx->type());
	});
}