module;
#include "hv/HThread.h"
#include "hv/HttpMessage.h"
#include "hv/HttpService.h"
#include "hv/hurl.h"

#include <any>
export module ApiManager:ApiLogin;

import AuthServerHelper;

using namespace std;
using namespace hv;

export void ApiLogin(HttpService* service)
{
	service->POST("/Login/Auth", [](const HttpContextPtr& ctx) 
	{
		hv::Json data;
		if(ctx->is(CONTENT_TYPE_NONE))
		{
			data["code"] = HTTP_STATUS_BAD_REQUEST;
			data["message"] = "Req contentType err!";
			return ctx->send(data.dump());
		}

		ctx->setContentType(ctx->type());
		
		auto authServer = GetAuthServer();
		if(!authServer)
		{
			data["code"] = HTTP_STATUS_BAD_REQUEST;
			data["message"] = "Server NotInitail!";
			return ctx->send(data.dump(), ctx->type());
		}
		
		if(false && !authServer->GetCSock()->IsRegisted())
		{
			data["code"] = HTTP_STATUS_BAD_REQUEST;
			data["message"] = "Server Disconnect!";
			return ctx->send(data.dump(), ctx->type());
		}

		string username = ctx->get("username");
    	string password = ctx->get("password");

		if(username.empty() || password.empty())
		{
			data["code"] = HTTP_STATUS_BAD_REQUEST;
			data["message"] = "param error!";
			return ctx->send(data.dump(), ctx->type());
		}

		data["code"] = HTTP_STATUS_BAD_REQUEST;

		data["data"]  =
		{
			{"timespan"	, 50505005005	},
			{"token"	, 3.140225005	},
			{"servPort"	, 90			},
			{"userId"	, "hello"		},
			{"servIp"	, "127.0.0.1"	},
			{"roleList"	, {127,265}	},
			{"rolemap"	, { {"job", {1,1}},{"sex" ,{1,2}}}},
		};

		return ctx->send(data.dump(), ctx->type());
	});


}