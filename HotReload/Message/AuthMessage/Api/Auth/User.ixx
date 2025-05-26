module;
export module ApiManager:ApiAuth;

import DNTask;
import FuncHelper;
import DllUtils;
import DbUtils;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ThirdParty.Libpqxx;
import std.compat;
import DNServer;
import AuthServerHelper;

using namespace std::chrono;

#define MSGSET writer->response->SetBody

export void ApiAuth(DNServer::WPtr server, hv::HttpService* service)
{
	service->POST("/Auth/User/LoginToken", [server](const hv::HttpRequestPtr& req, const hv::HttpResponseWriterPtr& writer)
		{
			writer->Begin();
			nlohmann::json errData;

			std::string authName = req->GetString("authName");
			std::string authString = req->GetString("authString");

			if (authName.empty() || authName.size() > 32 ||
				authString.empty() || authString.size() > 64)
			{
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "param error!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}

			GDb::Account accInfo;
			accInfo.set_auth_name(authName);
			accInfo.set_auth_string(authString);

			DNServer::Ptr serverTemp = server.lock();
			if (!serverTemp)
			{
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Server Disconnect!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}

			AuthServerHelper::Ptr dnServer = serverTemp->GetSelf<AuthServerHelper>();

			try
			{
				
				std::shared_ptr<pqxx::connection> connection = dnServer->GetRdbProxy()->GetConnection(static_cast<uint16_t>(EMSqlDbNameEnum::Account));

				pqxx::read_transaction query(*connection);
				DbSqlHelper<GDb::Account> accounts(&query);

				#define DBSelectOne(obj, name) .SelectOne(#name, [&obj]() { return obj.name(); })
				#define DBSelectCond(obj, name, cond, splicing) .SelectCond(#name, cond, splicing, [&obj]() { return obj.name(); })
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
			catch (const std::exception& e)
			{
				dnServer->GetLogger()->Record(ELogLevel_Debug, e.what());
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Server Error!!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}

			auto taskGen = [dnServer](GDb::Account accInfo, hv::HttpResponseWriterPtr writer) -> DNTaskVoid
				{
					// HttpResponseWriterPtr writer = writer;	//sharedptr ref count ++
					GMsg::A2g_ReqAuthAccount request;
					request.set_account_id(accInfo.account_id());
					request.set_server_ip(writer->peeraddr());

					GMsg::g2A_ResAuthAccount response;

					DNClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();

					// pack data
					std::string binData;
					request.SerializeToString(&binData);
					

					nlohmann::json retData;

					{
						// data alloc
						auto taskGen = [](Message* msg) -> DNTask<Message*>
							{
								co_return msg;
							};

						auto dataChannel = taskGen(&response);
						
						uint32_t msgId = clientProxy->GetMsgId();
						clientProxy->AddMsg(msgId, &dataChannel);
						MessagePackAndSend(msgId, EMMsgDeal::Redir, request.GetDescriptor()->full_name(), binData, clientProxy->GetChannel());
						
						co_await dataChannel;
						if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
						{
							retData["code"] = HTTP_STATUS_REQUEST_TIMEOUT;

							response.set_error_code(EL10nCode_SAuthReqTimeout);
						}
						else
						{
							retData["code"] = HTTP_STATUS_OK;
						}

					}

					binData.clear();
					auto state = MessageToJsonString(response, &binData);
					retData["data"] = nlohmann::json::parse(binData);
					retData["data"]["accountId"] = accInfo.account_id();

					MSGSET(retData.dump());
					writer->End();

					co_return;
				};

			taskGen(accInfo, writer);
		});

	service->POST("/Auth/User/RegistUser", [server](const hv::HttpRequestPtr& req, const hv::HttpResponseWriterPtr& writer)
		{
			nlohmann::json errData;

			std::string authName = req->GetString("authName");
			std::string authString = req->GetString("authString");

			if (authName.empty() || authName.size() > 32 ||
				authString.empty() || authString.size() > 64)
			{
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "param error!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}


			GDb::Account accInfo;
			accInfo.set_auth_name(authName);
			accInfo.set_auth_string(authString);

			DNServer::Ptr serverTemp = server.lock();
			if (!serverTemp)
			{
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Server Disconnect!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}

			AuthServerHelper::Ptr dnServer = serverTemp->GetSelf<AuthServerHelper>();

			try
			{
				std::shared_ptr<pqxx::connection> connection = dnServer->GetRdbProxy()->GetConnection(static_cast<uint16_t>(EMSqlDbNameEnum::Account));
				
				pqxx::read_transaction query(*connection);
				DbSqlHelper<GDb::Account> accounts(&query);

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
			catch (const std::exception& e)
			{
				dnServer->GetLogger()->Record(ELogLevel_Debug, e.what());
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
				std::shared_ptr<pqxx::connection> connection = dnServer->GetRdbProxy()->GetConnection(static_cast<uint16_t>(EMSqlDbNameEnum::Account));

				pqxx::work query(*connection);
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
			catch (const std::exception& e)
			{
				dnServer->GetLogger()->Record(ELogLevel_Debug, e.what());
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Regist Error!!";
				MSGSET(errData.dump());
			}

			writer->End();
		});


	service->POST("/Auth/Test/DB", [server](const hv::HttpRequestPtr& req, const hv::HttpResponseWriterPtr& writer)
		{

		});
}