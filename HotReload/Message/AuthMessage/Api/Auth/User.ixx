module;
#include <coroutine>
#include <cstdint>
#include "google/protobuf/util/json_util.h"
#include "hv/HttpService.h"
#include "pqxx/transaction"

#include "StdMacro.h"
#include "GDef/GDef.pb.h"
#include "Server/S_Auth_Control.pb.h"
export module ApiManager:ApiAuth;

import AuthServerHelper;
import DNTask;
import MessagePack;
import Macro;
import DNDbObj;
import Logger;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;
using namespace std::chrono;

#define MSGSET writer->response->SetBody

export void ApiAuth(HttpService* service)
{
	service->POST("/Auth/User/LoginToken", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer)
		{
			writer->Begin();
			hv::Json errData;

			string authName = req->GetString("authName");
			string authString = req->GetString("authString");

			if (authName.empty() || authName.size() > 32 ||
				authString.empty() || authString.size() > 64)
			{
				errData["code"] = HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "param error!";
				MSGSET(errData.dump());
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
				DNDbObj<GDb::Account> accounts(&query);

				accounts
					// DBSelect(accInfo, account_id)
					.SelectAll()
					DBSelectCond(accInfo, auth_name, "=", "")
					DBSelectCond(accInfo, auth_string, "=", " AND ")
					.Limit(2)
					.Commit();

				if (accounts.Result().size() != 1)
				{
					errData["code"] = HTTP_STATUS_BAD_REQUEST;
					errData["message"] = "not Account!";
					MSGSET(errData.dump());
					writer->End();
					return;
				}

				accInfo = accounts.Result()[0];
			}
			catch (const exception& e)
			{
				DNPrint(0, LoggerLevel::Debug, "%s", e.what());
				errData["code"] = HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Server Error!!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}

			auto taskGen = [](GDb::Account accInfo, HttpResponseWriterPtr writer) -> DNTaskVoid
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
					if (client->GetMsg(msgId))
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

					Json retData;

					{
						// data alloc
						auto taskGen = [](Message* msg) -> DNTask<Message*>
							{
								co_return msg;
							};
						auto dataChannel = taskGen(&response);
						// wait data parse
						client->AddMsg(msgId, &dataChannel);
						client->send(binData);
						co_await dataChannel;
						if (dataChannel.HasFlag(DNTaskFlag::Timeout))
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

					MSGSET(retData.dump());
					writer->End();

					co_return;
				};

			taskGen(accInfo, writer);
		});

	service->POST("/Auth/User/RegistUser", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer)
		{
			hv::Json errData;

			string authName = req->GetString("authName");
			string authString = req->GetString("authString");

			if (authName.empty() || authName.size() > 32 ||
				authString.empty() || authString.size() > 64)
			{
				errData["code"] = HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "param error!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}

			AuthServerHelper* authServer = GetAuthServer();

			GDb::Account accInfo;
			accInfo.set_auth_name(authName);
			accInfo.set_auth_string(authString);

			try
			{
				pqxx::read_transaction query(*authServer->SqlProxy());
				DNDbObj<GDb::Account> accounts(&query);

				accounts
					.SelectAll(false, true)
					DBSelectCond(accInfo, auth_name, "=", "")
					.Commit();

				if (uint32_t count = accounts.ResultCount())
				{
					errData["code"] = HTTP_STATUS_BAD_REQUEST;
					errData["message"] = "already exist authName!!";
					MSGSET(errData.dump());
					writer->End();
					return;
				}
			}
			catch (const exception& e)
			{
				DNPrint(0, LoggerLevel::Debug, "%s", e.what());
				errData["code"] = HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Regist Error!!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}

			double msTime = time_point_cast<microseconds>(system_clock::now()).time_since_epoch().count() / 1000000.0;

			accInfo.set_create_time(msTime);
			accInfo.set_update_time(msTime);
			accInfo.set_last_logout_time(msTime);

			try
			{

				pqxx::work query(*authServer->SqlProxy());
				DNDbObj<GDb::Account> accounts(&query);

				accounts.Insert(accInfo).Commit();

				query.commit();

				if (accounts.IsSuccess())
				{
					errData["code"] = HTTP_STATUS_OK;
					errData["message"] = "Regist Success!!";
					MSGSET(errData.dump());
				}
				else
				{
					errData["code"] = HTTP_STATUS_OK;
					errData["message"] = "Regist Error!!";
					MSGSET(errData.dump());
				}
			}
			catch (const exception& e)
			{
				DNPrint(0, LoggerLevel::Debug, "%s", e.what());
				errData["code"] = HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Regist Error!!";
				MSGSET(errData.dump());
			}

			writer->End();
		});

		
	service->POST("/Auth/Utils/ChangeIp", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer)
		{
			string ip = req->GetString("ip");
			int port = req->Get<int>("port", 0);

			if(ip.empty() || !port)
			{
				writer->End();
				return;
			}

			AuthServerHelper* authServer = GetAuthServer();
			DNClientProxyHelper* client = authServer->GetCSock();

			TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, RedirectClient, client, port, ip);

			writer->End();
		});
}