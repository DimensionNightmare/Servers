module;
#include "hv/HThread.h"
#include "hv/HttpMessage.h"
#include "hv/HttpService.h"
#include "hv/hurl.h"

export module ApiManager:ApiLogin;

import AuthServerHelper;

using namespace std;
using namespace hv;

export void ApiLogin(HttpService* service)
{
	service->POST("/Login/Auth", [](const HttpContextPtr& ctx) 
	{
		ctx->setContentType(ctx->type());
		
		auto authServer = GetAuthServer();
		if(!authServer)
		{
			ctx->set("code", HTTP_STATUS_BAD_REQUEST);
			ctx->set("message", "Server NotInitail!");
			return HTTP_STATUS_OK;
		}
		
		if(false && !authServer->GetCSock()->IsRegisted())
		{
			ctx->set("code", HTTP_STATUS_BAD_REQUEST);
			ctx->set("message", "Server Disconnect!");

			// ctx->response->json = ctx->request->GetJson();
			// ctx->response->json["int"] = 123;
			// ctx->response->json["float"] = 3.14;
			// ctx->response->json["string"] = "hello";

			// ctx->response->form = ctx->request->GetForm();
			// ctx->response->SetFormData("int", 123);
			// ctx->response->SetFormData("float", 3.14);
			// ctx->response->SetFormData("string", "hello");
			return HTTP_STATUS_OK;
		}

		std::string username = ctx->get("username");
    	std::string password = ctx->get("password");

		ctx->set("timespan", 50505005005);
		ctx->set("token", 3.140225005);
		ctx->set("userId", "hello");
		ctx->set("servIp", "127.0.0.1");
		ctx->set("servPort", 90);

		ctx->set("code", HTTP_STATUS_OK);
		ctx->set("message", "");
		return HTTP_STATUS_OK;
	});

	service->GET("/Login/Auth1", [](const HttpContextPtr& ctx) 
	{
		
		return HTTP_STATUS_OK;
	});
}