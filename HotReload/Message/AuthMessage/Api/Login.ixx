module;
#include <coroutine>
#include <cstdint>
#include "google/protobuf/util/json_util.h"
#include "hv/HttpService.h"
#include "pqxx/transaction"

#include "StdAfx.h"
#include "DbAfx.h"
#include "GDef/GDef.pb.h"
#include "Server/S_Auth_Control.pb.h"
export module ApiManager:ApiLogin;

import AuthServerHelper;
import DNTask;
import MessagePack;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

export void ApiLogin(HttpService* service)
{
	service->POST("/Auth/LoginToken", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer) 
	{
		writer->Begin();
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
		
		AuthServerHelper* authServer = GetAuthServer();
		if(!authServer)
		{
			errData["code"] = HTTP_STATUS_BAD_REQUEST;
			errData["message"] = "Server NotInitail!";
			res->SetBody(errData.dump());
			writer->End();
			return;
		}
		
		if(authServer->GetCSock()->RegistState() != RegistState::Registed)
		{
			errData["code"] = HTTP_STATUS_BAD_REQUEST;
			errData["message"] = "Server Disconnect!";
			res->SetBody(errData.dump());
			writer->End();
			return;
		}

		string authName = req->GetString("authName");
    	string authString = req->GetString("authString");

		if(authName.empty() || authString.empty())
		{
			errData["code"] = HTTP_STATUS_BAD_REQUEST;
			errData["message"] = "param error!";
			res->SetBody(errData.dump());
			writer->End();
			return;
		}

		GDb::Account accInfo;
		accInfo.set_auth_name(authName);
		accInfo.set_auth_string(authString);

		pqxx::read_transaction query(*authServer->GetDbConnection());
		DNDbObj<GDb::Account> accounts(reinterpret_cast<pqxx::transaction<>*>(&query));
		try
		{
			accounts
				.SelectAll()
				DBSelectCond(accInfo, auth_name, "=", "")
				DBSelectCond(accInfo, auth_string, "=", "AND")
				.Commit();
		}
		catch(exception& e)
		{
			DNPrint(0, LoggerLevel::Debug, "%s", e.what());
			errData["code"] = HTTP_STATUS_BAD_REQUEST;
			errData["message"] = "Server Error!!";
			res->SetBody(errData.dump());
			writer->End();
			return;
		}
		

		if(accounts.Result().size() != 1) 
		{
			errData["code"] = HTTP_STATUS_BAD_REQUEST;
			errData["message"] = "not Account!";
			res->SetBody(errData.dump());
			writer->End();
			return;
		}

		accInfo = accounts.Result()[0];


		[accInfo, writer]()-> DNTaskVoid
		{
			const HttpResponseWriterPtr& writerProxy = writer;	//sharedptr ref count ++
			A2C_ReqAuthAccount requset;
			requset.set_account_id(accInfo.account_id());
			requset.set_ip(writerProxy->peeraddr());

			C2A_ResAuthAccount response;
			
			AuthServerHelper* authServer = GetAuthServer();
			DNClientProxyHelper* client = authServer->GetCSock();
			uint32_t msgId = client->GetMsgId();
			
			// first Can send Msg?
			if(client->GetMsg(msgId))
			{
				DNPrint(0, LoggerLevel::Debug, "+++++ %lu, \n", msgId);
				co_return;
			}
			// else
			// {
				DNPrint(0, LoggerLevel::Debug, "----- %lu, \n", msgId);
			// }
			
			// pack data
			string binData;
			binData.resize(requset.ByteSizeLong());
			requset.SerializeToArray(binData.data(), binData.size());
			MessagePack(msgId, MsgDeal::Req, requset.GetDescriptor()->full_name().c_str(), binData);
			
			// data alloc
			auto dataChannel = [&response]()->DNTask<Message>
			{
				co_return response;
			}();

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

			Json retData;

			{
				// wait data parse
				client->AddMsg(msgId, &dataChannel);
				client->send(binData);
				co_await dataChannel;
				if(dataChannel.HasFlag(DNTaskFlag::Timeout))
				{
					retData["code"] = HTTP_STATUS_OK;
				}
				else
				{
					retData["code"] = HTTP_STATUS_REQUEST_TIMEOUT;
				}
			}

			writerProxy->onclose = nullptr;

			// response.set_ip_addr("asdasda");
			// response.set_token("asdas");
			// response.set_state_code(958);
			// response.set_expired_timespan(65664);
			binData.clear();
			util::MessageToJsonString(response, &binData);
			retData["data"] = Json::parse(binData);
			retData["data"]["accountId"] = accInfo.account_id();
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