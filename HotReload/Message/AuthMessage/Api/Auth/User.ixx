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
export module ApiManager:ApiAuth;

import AuthServerHelper;
import DNTask;
import MessagePack;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

export void ApiAuth(HttpService* service)
{
	service->POST("/Auth/User/LoginToken", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer) 
	{
		writer->Begin();
		HttpResponsePtr res = writer->response;
		hv::Json errData;

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

		try
		{
			AuthServerHelper* authServer = GetAuthServer();
			pqxx::read_transaction query(*authServer->SqlProxy());
			DNDbObj<GDb::Account> accounts(reinterpret_cast<pqxx::transaction<>*>(&query));

			accounts
				.SelectAll()
				DBSelectCond(accInfo, auth_name, "=", "")
				DBSelectCond(accInfo, auth_string, "=", "AND")
				.Commit();

			if(accounts.Result().size() != 1) 
			{
				errData["code"] = HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "not Account!";
				res->SetBody(errData.dump());
				writer->End();
				return;
			}

			accInfo = accounts.Result()[0];
		}
		catch(const exception& e)
		{
			DNPrint(0, LoggerLevel::Debug, "%s", e.what());
			errData["code"] = HTTP_STATUS_BAD_REQUEST;
			errData["message"] = "Server Error!!";
			res->SetBody(errData.dump());
			writer->End();
			return;
		}
		
		auto taskGen = [accInfo](HttpResponseWriterPtr writer)-> DNTaskVoid
		{
			// HttpResponseWriterPtr writer = writer;	//sharedptr ref count ++
			A2C_ReqAuthAccount requset;
			requset.set_account_id(accInfo.account_id());
			requset.set_ip(writer->peeraddr());

			C2A_ResAuthAccount response;
			
			AuthServerHelper* authServer = GetAuthServer();
			DNClientProxyHelper* client = authServer->GetCSock();
			uint32_t msgId = client->GetMsgId();
			
			// first Can send Msg?
			if(client->GetMsg(msgId))
			{
				DNPrint(0, LoggerLevel::Debug, "+++++ %lu, ", msgId);
				co_return;
			}
			// else
			// {
				DNPrint(0, LoggerLevel::Debug, "----- %lu, ", msgId);
			// }
			
			// pack data
			string binData;
			binData.resize(requset.ByteSizeLong());
			requset.SerializeToArray(binData.data(), binData.size());
			MessagePack(msgId, MsgDeal::Req, requset.GetDescriptor()->full_name().c_str(), binData);
			
			// data alloc
			auto taskGen = [&response]()->DNTask<Message>
			{
				co_return response;
			};

			auto dataChannel = taskGen();

			Json retData;

			{
				// wait data parse
				client->AddMsg(msgId, &dataChannel);
				client->send(binData);
				co_await dataChannel;
				if(dataChannel.HasFlag(DNTaskFlag::Timeout))
				{
					retData["code"] = HTTP_STATUS_REQUEST_TIMEOUT;
					
					response.set_state_code(1);
				}
				else
				{
					retData["code"] = HTTP_STATUS_OK;
				}
			}

			binData.clear();
			util::MessageToJsonString(response, &binData);
			retData["data"] = Json::parse(binData);
			retData["data"]["accountId"] = accInfo.account_id();

			writer->response->SetBody(retData.dump());
			writer->End();

			dataChannel.Destroy();

			co_return;
		};

		taskGen(writer);

	});

	service->POST("/Auth/User/RegistUser", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer) 
	{
		string auth = req->GetString("value");
		cout  << auth << endl;
		writer->response->SetBody("ads");

		writer->End();
	});

}