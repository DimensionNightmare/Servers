module;
#include "AuthControl.pb.h"
#include "hv/HThread.h"
#include "hv/HttpMessage.h"
#include "hv/HttpService.h"
#include "hv/hurl.h"

#include <coroutine>
export module ApiManager:ApiLogin;

import AuthServerHelper;
import DNTask;
import MessagePack;
import AfxCommon;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr(), __FUNCTION__, ##__VA_ARGS__);

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::AuthControl;

static bool statues = false;

DNTaskVoid ReqDataFromControlServer(Message& requset, Message& response)
{
	auto authServer = GetAuthServer();
	auto client = authServer->GetCSock();
	auto msgId = client->GetMsgId();
	auto& reqMap = client->GetMsgMap();
	
	// first Can send Msg?
	if(reqMap.contains(msgId))
	{
		DNPrintErr("+++++ %lu, \n", msgId);
		co_return;
	}
	else
	{
		DNPrint("----- %lu, \n", msgId);
	}
	
	// pack data
	string binData;
	binData.resize(requset.ByteSizeLong());
	requset.SerializeToArray(binData.data(), (int)binData.size());
	MessagePack(msgId, MsgDeal::Req, requset.GetDescriptor()->full_name(), binData);
	
	// data alloc
	auto dataChannel = [&]()->DNTask<Message*>
	{
		co_return &response;
	}();

	reqMap.emplace(msgId, (DNTask<void*>*)&dataChannel);
	
	// wait data parse
	client->send(binData);
	co_await dataChannel;
	
	dataChannel.Destroy();

	co_return;
};

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
		
		if(!authServer->GetCSock()->IsRegisted())
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

		A2C_AuthAccount requset;
		requset.set_username(username);
		requset.set_password(password);

		C2A_AuthAccount response;
		auto handle = ReqDataFromControlServer(requset, response);

		auto status = handle.await_ready();
		// int a = 0;
		while(!handle.await_ready())
		{
			// printf("asdasdasd%d\n",a++);
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