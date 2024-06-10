module;
#include <coroutine>
#include <cstdint>
#include <format>
#include <chrono>
#include "google/protobuf/util/json_util.h"
#include "hv/HttpService.h"
#include "pqxx/transaction"

#include "StdMacro.h"
#include "GDef/GDef.pb.h"
#include "Server/S_Auth.pb.h"
export module ApiManager:ApiAuth;

import AuthServerHelper;
import DNTask;
import MessagePack;
import Macro;
import DbUtils;
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
				DbSqlHelper<GDb::Account> accounts(&query);

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
					errData["code"] = HTTP_STATUS_BAD_REQUEST;
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
				errData["code"] = HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Server Error!!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}

			auto taskGen = [](GDb::Account accInfo, HttpResponseWriterPtr writer) -> DNTaskVoid
				{
					// HttpResponseWriterPtr writer = writer;	//sharedptr ref count ++
					A2g_ReqAuthAccount request;
					request.set_account_id(accInfo.account_id());
					request.set_ip(writer->peeraddr());

					g2A_ResAuthAccount response;

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
					request.SerializeToString(&binData);
					MessagePack(msgId, MsgDeal::Redir, request.GetDescriptor()->full_name().c_str(), binData);

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
				DbSqlHelper<GDb::Account> accounts(&query);

				accounts
					.InitEntity(accInfo)
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

			int64_t msTime = time_point_cast<nanoseconds>(system_clock::now()).time_since_epoch().count();

			accInfo.set_create_time(msTime);
			accInfo.set_update_time(msTime);
			accInfo.set_last_logout_time(msTime);
			accInfo.set_last_logout_time(msTime);

			try
			{

				pqxx::work query(*authServer->SqlProxy());
				DbSqlHelper<GDb::Account> accounts(&query);

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
				errData["code"] = HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Regist Error!!";
				MSGSET(errData.dump());
			}

			writer->End();
		});


	service->POST("/Auth/Test/MainSpace", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer)
		{
			string ip = req->GetString("ip");
			int port = req->Get<int>("port", 0);

			if (ip.empty() || !port)
			{
				writer->End();
				return;
			}

			AuthServerHelper* authServer = GetAuthServer();
			DNClientProxyHelper* client = authServer->GetCSock();

			TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, RedirectClient, client, port, ip);

			writer->End();
		});

	service->POST("/Auth/Test/Chrono", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer)
		{
			system_clock::time_point now = system_clock::now();
			string strTIme = format("{:%Y-%m-%d %H:%M:%S}", now);
			string strTIme1 = format("{:%Y-%m-%d %H:%M:%S}", zoned_time(current_zone(), now));
			hv::Json errData = {
				{ "milliseconds" , time_point_cast<milliseconds>(now).time_since_epoch().count()},
				{ "microseconds" , time_point_cast<microseconds>(now).time_since_epoch().count()},
				{ "nanoseconds" , time_point_cast<nanoseconds>(now).time_since_epoch().count()},
				{ "string" ,  strTIme},
				{"zone", strTIme1},
			};

			MSGSET(errData.dump());
			writer->End();
		});


	service->POST("/Auth/Test/DB", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer)
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


			GDb::Account accInfo;
			accInfo.set_auth_name(authName);
			accInfo.set_auth_string(authString);

			try
			{
				AuthServerHelper* authServer = GetAuthServer();
				pqxx::work query(*authServer->SqlProxy());
				DbSqlHelper<GDb::Account> accounts(&query);

				// accounts.Insert(accInfo).Commit();

				// query.commit();

				// if (accounts.IsSuccess())
				// {
				// 	errData["code"] = HTTP_STATUS_OK;
				// 	errData["message"] = "Regist Success!!";
				// 	MSGSET(errData.dump());
				// }
				// else
				// {
				// 	errData["code"] = HTTP_STATUS_OK;
				// 	errData["message"] = "Regist Error!!";
				// 	MSGSET(errData.dump());
				// }

				accounts
					.SelectAll()
					DBSelectCond(accInfo, auth_name, "=", "")
					.Commit();

				for (const GDb::Account* one : accounts.Result())
				{
					errData["code"] = HTTP_STATUS_OK;
					errData["message"] = one->DebugString();
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
			}

			writer->End();
		});
}