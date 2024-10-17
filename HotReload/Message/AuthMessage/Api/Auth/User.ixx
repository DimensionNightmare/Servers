module;
#include "hv/json.hpp"
#include "StdMacro.h"
export module ApiManager:ApiAuth;

import AuthServerHelper;
import DNTask;
import FuncHelper;
import Macro;
import DbUtils;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ThirdParty.Libpqxx;

using namespace std::chrono;

#define MSGSET writer->response->SetBody

export void ApiAuth(HttpService* service)
{
	service->POST("/Auth/User/LoginToken", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer)
		{
			writer->Begin();
			nlohmann::json errData;

			string authName = req->GetString("authName");
			string authString = req->GetString("authString");

			if (authName.empty() || authName.size() > 32 ||
				authString.empty() || authString.size() > 64)
			{
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "param error!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}

			Account accInfo;
			accInfo.set_auth_name(authName);
			accInfo.set_auth_string(authString);

			try
			{
				AuthServerHelper* authServer = GetAuthServer();
				read_transaction query(*authServer->SqlProxy());
				DbSqlHelper<Account> accounts(&query);

				accounts
					// DBSelectOne(accInfo, account_id)
					.InitEntity(accInfo)
					.SelectAll()
					DBSelectCond(accInfo, auth_name, "=", "")
					DBSelectCond(accInfo, auth_string, "=", " AND ")
					.Limit(2)
					.Commit();

				if (accounts.Result().size() != 1)
				{
					errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
					errData["message"] = "not Account!";
					MSGSET(errData.dump());
					writer->End();
					return;
				}

				accInfo = *accounts.Result()[0];
			}
			catch (const exception& e)
			{
				DNPrint(0, LoggerLevel::Debug, "%s", e.what());
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Server Error!!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}

			auto taskGen = [](Account accInfo, HttpResponseWriterPtr writer) -> DNTaskVoid
				{
					// HttpResponseWriterPtr writer = writer;	//sharedptr ref count ++
					A2g_ReqAuthAccount request;
					request.set_account_id(accInfo.account_id());
					request.set_server_ip(writer->peeraddr());

					g2A_ResAuthAccount response;

					AuthServerHelper* authServer = GetAuthServer();
					DNClientProxyHelper* client = authServer->GetCSock();

					// pack data
					string binData;
					request.SerializeToString(&binData);
					

					nlohmann::json retData;

					{
						// data alloc
						auto taskGen = [](Message* msg) -> DNTask<Message*>
							{
								co_return msg;
							};

						auto dataChannel = taskGen(&response);
						
						uint32_t msgId = client->GetMsgId();
						client->AddMsg(msgId, &dataChannel);
						MessagePackAndSend(msgId, MsgDeal::Redir, request.GetDescriptor()->full_name().c_str(), binData, client->GetChannel());
						
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
					auto state = PBExport::MessageToJsonString(response, &binData);
					retData["data"] = nlohmann::json::parse(binData);
					retData["data"]["accountId"] = accInfo.account_id();

					MSGSET(retData.dump());
					writer->End();

					co_return;
				};

			taskGen(accInfo, writer);
		});

	service->POST("/Auth/User/RegistUser", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer)
		{
			nlohmann::json errData;

			string authName = req->GetString("authName");
			string authString = req->GetString("authString");

			if (authName.empty() || authName.size() > 32 ||
				authString.empty() || authString.size() > 64)
			{
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "param error!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}

			AuthServerHelper* authServer = GetAuthServer();

			Account accInfo;
			accInfo.set_auth_name(authName);
			accInfo.set_auth_string(authString);

			try
			{
				read_transaction query(*authServer->SqlProxy());
				DbSqlHelper<Account> accounts(&query);

				accounts
					.InitEntity(accInfo)
					.SelectAll(false, true)
					DBSelectCond(accInfo, auth_name, "=", "")
					.Commit();

				if (uint32_t count = accounts.ResultCount())
				{
					errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
					errData["message"] = "already exist authName!!";
					MSGSET(errData.dump());
					writer->End();
					return;
				}
			}
			catch (const exception& e)
			{
				DNPrint(0, LoggerLevel::Debug, "%s", e.what());
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Regist Error!!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}

			int64_t msTime = time_point_cast<nanoseconds>(system_clock::now()).time_since_epoch().count();

			accInfo.set_create_time(msTime);
			accInfo.set_update_time(msTime);
			accInfo.set_last_logout_time(msTime);
			accInfo.set_last_logout_time(msTime);

			try
			{

				pq_work query(*authServer->SqlProxy());
				DbSqlHelper<Account> accounts(&query);

				accounts.InitEntity(accInfo).Insert().Commit();

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
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Regist Error!!";
				MSGSET(errData.dump());
			}

			writer->End();
		});


	service->POST("/Auth/Test/DB", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer)
		{

		});
}