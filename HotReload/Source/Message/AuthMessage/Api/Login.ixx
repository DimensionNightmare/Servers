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
	service->GET("/ping", [](HttpRequest* req, HttpResponse* resp) {
		return resp->String("pong");
	});

	service->GET("/data", [](HttpRequest* req, HttpResponse* resp) {
		static char data[] = "0123456789";
		return resp->Data(data, 10 /*, false */);
	});

	service->GET("/paths", [service](HttpRequest* req, HttpResponse* resp) {
		return resp->Json(service->Paths());
	});

	service->GET("/get", [](const HttpContextPtr& ctx) {
		hv::Json resp;
		resp["origin"] = ctx->ip();
		resp["url"] = ctx->url();
		resp["args"] = ctx->params();
		resp["headers"] = ctx->headers();
		return ctx->send(resp.dump(2));
	});

	service->POST("/echo", [](const HttpContextPtr& ctx) {
		return ctx->send(ctx->body(), ctx->type());
	});

	service->GET("/user/{id}/{zcz}", [](const HttpContextPtr& ctx) {
		hv::Json resp;
		resp["id"] = ctx->param("id");
		resp["param"] = HUrl::unescape(ctx->param("zcz"));
		return ctx->send(resp.dump(2));
	});

	service->GET("/async", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer) {
		writer->Begin();
		writer->WriteHeader("X-Response-tid", hv_gettid());
		writer->WriteHeader("Content-Type", "text/plain");
		writer->WriteBody("This is an async response.\n");
		writer->End();
	});
}