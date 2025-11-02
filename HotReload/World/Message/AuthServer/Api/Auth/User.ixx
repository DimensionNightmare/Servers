export module ApiManager:ApiAuth;

import DbUtils;
import FuncHelper;
import AuthServerHelper;
import Server;
import std.compat;
import Task;
import WebProxyHelper;
import ThirdParty.PbGen;
import Logger;

using namespace std::chrono;

#define MSGSET writer->response->SetBody

std::atomic<size_t> counter = 0;

TaskVoid Func0()
{
	std::cout << "Task 999 exec" << "\n";
	co_return;
}

TaskVoid Func1()
{
	co_await Func0();
	std::cout << "Task 1 exec" << "\n";

	co_return;
}

TaskVoid Func2()
{
	std::cout << "Task 2 exec " << "\n";
	co_await Func1();
	co_return;
}


TaskVoid Func3()
{
	std::cout << "Task 3 exec" << "\n";
	co_await Func2();
	co_return;
}

TaskVoid Func4()
{
	std::cout << "Task 4 exec" << "\n";
	co_await Func3();
	co_return;
}

export void ApiAuth(Server::Ptr server)
{

	WebProxyHelper::Ptr webProxyHelper = server->GetComponent<WebProxyHelper>(EMComponentType::WebProxy);

	webProxyHelper->service->POST("/Auth/User/LoginToken", [server](hv::HttpRequestPtr req, hv::HttpResponseWriterPtr writer) ->TaskVoid
		{
			
			nlohmann::json errData;

			std::string authName = req->GetString("AuthName");
			std::string authString = req->GetString("AuthString");

			if (authName.empty() || authName.size() > 32 ||
				authString.empty() || authString.size() > 64)
			{
				writer->Begin();
				errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["Message"] = "param error!";
				MSGSET(errData.dump());
				writer->End();
				co_return;
			}

			GDb::Account accInfo;
			accInfo.set_authname(authName);
			accInfo.set_authstring(authString);

			if (!server)
			{
				writer->Begin();
				errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["Message"] = "Server Disconnect!";
				MSGSET(errData.dump());
				writer->End();
				co_return;
			}

			AuthServerHelper::Ptr dnServer = server->GetSelf<AuthServerHelper>();

			try
			{
				
				std::shared_ptr<pqxx::connection> connection = dnServer->GetRdbProxy()->GetConnection(EMSqlDbNameEnum::Account);

				if(!connection)
				{
					writer->Begin();
					errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
					errData["Message"] = "RDB Disconnect!";
					MSGSET(errData.dump());
					writer->End();
					co_return;
				}

				pqxx::read_transaction query(*connection);
				DbSqlHelper<GDb::Account> accounts(&query, dnServer->GetWorld());

				accounts
					.InitEntity(accInfo)
					.SelectAll()
					.SelectCond<GDb::Account::kAuthNameFieldNumber>("=", "")
					.SelectCond<GDb::Account::kAuthStringFieldNumber>("=", " AND ")
					.Limit(2)
					.Commit();

				if (accounts.Result().size() != 1)
				{
					writer->Begin();
					errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
					errData["Message"] = "not Account!";
					MSGSET(errData.dump());
					writer->End();
					co_return;
				}

				accInfo = *accounts.Result()[0];
			}
			catch (const std::exception& e)
			{
				writer->Begin();
				LoggerPrint::Log(server, ELogLevel_Debug, "{}", e.what());
				errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["Message"] = "Server Error!!";
				MSGSET(errData.dump());
				writer->End();
				co_return;
			}

			GMsg::A2g_ReqAuthAccount request;
			request.set_accountid(accInfo.accountid());
			request.set_serverip(writer->peeraddr());

			GMsg::g2A_ResAuthAccount response;

			ClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();

	
			nlohmann::json retData;

			bool success = co_await clientProxy->AddMsg(EMMsgDeal::Redir, &request, &response);
			
			if (!success)
			{
				retData["Code"] = HTTP_STATUS_REQUEST_TIMEOUT;

				response.set_errorcode(EL10nCode_SAuthReqTimeout);
			}
			else
			{
				retData["Code"] = HTTP_STATUS_OK;
			}

			if(response.errorcode() == EL10nCode_None)
			{
				std::string binData;
				auto state = MessageToJsonString(response, &binData);
				retData["Data"] = nlohmann::json::parse(binData);
				retData["Data"]["AccountId"] = accInfo.accountid();
			}
			else
			{
				retData["ErrorMessage"] = EL10nCode_Name(response.errorcode());
			}

			writer->Begin();
			MSGSET(retData.dump());
			writer->End();

			co_return;
				
		});

	webProxyHelper->service->POST("/Auth/User/RegistUser", [server](hv::HttpRequestPtr req, hv::HttpResponseWriterPtr writer)
		{
			nlohmann::json errData;

			std::string authName = req->GetString("AuthName");
			std::string authString = req->GetString("AuthString");

			if (authName.empty() || authName.size() > 32 ||
				authString.empty() || authString.size() > 64)
			{
				errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["Message"] = "param error!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}


			GDb::Account accInfo;
			accInfo.set_authname(authName);
			accInfo.set_authstring(authString);

			if (!server)
			{
				errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["Message"] = "Server Disconnect!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}

			AuthServerHelper::Ptr dnServer = server->GetSelf<AuthServerHelper>();

			try
			{
				std::shared_ptr<pqxx::connection> connection = dnServer->GetRdbProxy()->GetConnection(EMSqlDbNameEnum::Account);

				if(!connection)
				{
					writer->Begin();
					errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
					errData["Message"] = "RDB Disconnect!";
					MSGSET(errData.dump());
					writer->End();
					return;
				}
				
				pqxx::read_transaction query(*connection);
				DbSqlHelper<GDb::Account> accounts(&query, dnServer->GetWorld());

				accounts
					.InitEntity(accInfo)
					.SelectAll(false, true)
					.SelectCond<GDb::Account::kAuthNameFieldNumber>("=", "")
					.Commit();

				if (uint32_t count = accounts.ResultCount())
				{
					errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
					errData["Message"] = "already exist authName!!";
					MSGSET(errData.dump());
					writer->End();
					return;
				}
			}
			catch (const std::exception& e)
			{
				LoggerPrint::Log(server, ELogLevel_Debug, "{}", e.what());
				errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["Message"] = "Regist Error!!";
				MSGSET(errData.dump());
				writer->End();
				return;
			}

			int64_t msTime = time_point_cast<nanoseconds>(system_clock::now()).time_since_epoch().count();

			accInfo.set_createtime(msTime);
			accInfo.set_updatetime(msTime);
			accInfo.set_lastlogouttime(msTime);
			accInfo.set_lastlogouttime(msTime);

			try
			{
				std::shared_ptr<pqxx::connection> connection = dnServer->GetRdbProxy()->GetConnection(EMSqlDbNameEnum::Account);

				if(!connection)
				{
					writer->Begin();
					errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
					errData["Message"] = "RDB Disconnect!";
					MSGSET(errData.dump());
					writer->End();
					return;
				}

				pqxx::work query(*connection);
				DbSqlHelper<GDb::Account> accounts(&query, dnServer->GetWorld());

				accounts.InitEntity(accInfo).Insert().Commit();

				query.commit();

				if (accounts.IsSuccess())
				{
					errData["Code"] = HTTP_STATUS_OK;
					errData["Message"] = "Regist Success!!";
					MSGSET(errData.dump());
				}
				else
				{
					errData["Code"] = HTTP_STATUS_OK;
					errData["Message"] = "Regist Error!!";
					MSGSET(errData.dump());
				}
			}
			catch (const std::exception& e)
			{
				LoggerPrint::Log(server, ELogLevel_Debug, "{}", e.what());
				errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["Message"] = "Regist Error!!";
				MSGSET(errData.dump());
			}

			writer->End();
		});

	webProxyHelper->service->POST("/Auth/Test/User", [server](hv::HttpRequestPtr req, hv::HttpResponseWriterPtr writer)->TaskVoid
		{
			std::cout << "begin" << "\n";
			co_await Func4();
			writer->End();
			std::cout << "end" << "\n";
			co_return;
		});
}