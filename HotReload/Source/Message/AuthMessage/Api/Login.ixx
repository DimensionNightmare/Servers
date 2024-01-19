module;
#include "google/protobuf/util/json_util.h"
#include "AuthControl.pb.h"
#include "hv/HThread.h"
#include "hv/HttpMessage.h"
#include "hv/HttpService.h"
#include "hv/hurl.h"

#include <coroutine>
#include <thread>
#include <chrono>
export module ApiManager:ApiLogin;

import AuthServerHelper;
import DNTask;
import MessagePack;
import AfxCommon;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::AuthControl;

export void ApiLogin(HttpService* service)
{
	service->POST("/Login/Auth", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer) 
	{
		HttpResponsePtr res = writer->response;
		hv::Json errData;
		if(req->ContentType() == CONTENT_TYPE_NONE)
		{
			errData["code"] = HTTP_STATUS_BAD_REQUEST;
			errData["message"] = "Req contentType err!";
			res->content_type = APPLICATION_JSON;
			res->SetBody(errData.dump());
			writer->End();
			return;
		}

		res->content_type = req->ContentType();
		
		auto authServer = GetAuthServer();
		if(!authServer)
		{
			errData["code"] = HTTP_STATUS_BAD_REQUEST;
			errData["message"] = "Server NotInitail!";
			res->SetBody(errData.dump());
			writer->End();
			return;
		}
		
		if(!authServer->GetCSock()->IsRegisted())
		{
			errData["code"] = HTTP_STATUS_BAD_REQUEST;
			errData["message"] = "Server Disconnect!";
			res->SetBody(errData.dump());
			writer->End();
			return;
		}

		string username = req->GetString("username");
    	string password = req->GetString("password");

		if(username.empty() || password.empty())
		{
			errData["code"] = HTTP_STATUS_BAD_REQUEST;
			errData["message"] = "param error!";
			res->SetBody(errData.dump());
			writer->End();
			return;
		}

		[username,password,&writer]()-> DNTaskVoid
		{
			auto writerProxy = writer;	//sharedptr ref count ++
			A2C_AuthAccount requset;
			requset.set_username(username);
			requset.set_password(password);

			C2A_AuthAccount response;
			
			auto authServer = GetAuthServer();
			auto client = authServer->GetCSock();
			auto msgId = client->GetMsgId();
			
			// first Can send Msg?
			if(client->GetMsg(msgId))
			{
				DNPrintErr("+++++ %lu, \n", msgId);
				co_return;
			}
			// else
			// {
				printf("----- %lu, \n", msgId);
			// }
			
			// pack data
			string binData;
			binData.resize(requset.ByteSizeLong());
			requset.SerializeToArray(binData.data(), (int)binData.size());
			MessagePack(msgId, MsgDeal::Req, requset.GetDescriptor()->full_name(), binData);
			
			// data alloc
			auto dataChannel = [&response]()->DNTask<Message*>
			{
				co_return &response;
			}();

			client->AddMsg(msgId, (DNTask<void*>*)&dataChannel);

			// regist Close event to release memory
			if(writerProxy->onclose)
			{
				writerProxy->onclose();
				writerProxy->onclose = nullptr;
			}

			writerProxy->onclose = [&dataChannel, client, msgId]()
			{
				client->DelMsg(msgId);
				dataChannel.CallResume();
			};
			
			// wait data parse
			client->send(binData);
			co_await dataChannel;

			writerProxy->onclose = nullptr;

			Json retData;

			retData["code"] = HTTP_STATUS_OK;
			response.set_ip_addr("asdasda");
			response.set_token("asdas");
			response.set_state_code(958);
			response.set_expired_timespan(65664);
			binData.clear();
			util::MessageToJsonString(response, &binData);
			retData["data"] = Json::parse(binData);
			// {
			// 	{"timespan"	, 50505005005	},
			// 	{"token"	, 3.140225005	},
			// 	{"servPort"	, 90			},
			// 	{"userId"	, "hello"		},
			// 	{"servIp"	, "127.0.0.1"	},
			// 	{"roleList"	, {127,265}	},
			// 	{"rolemap"	, { {"job", {1,1}},{"sex" ,{1,2}}}},
			// };

			writerProxy->response->SetBody(retData.dump());

			writerProxy->End();

			dataChannel.Destroy();

			co_return;
		}();

	});


}