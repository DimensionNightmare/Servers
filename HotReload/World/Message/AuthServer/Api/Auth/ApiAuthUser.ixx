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

class A
{
public:
	A(){}

	std::string a = std::string("zzzzzzzzzccc");
};

TaskVoid Func0()
{
	std::cout << "Task 999 exec" << "\n";
	co_return;
}

MsgTask Func00()
{
	std::cout << "Task 999 exec" << "\n";
	co_return;
}

Task<std::shared_ptr<A>> Func1()
{
	std::shared_ptr<A> aa = std::shared_ptr<A>(new A);
	// co_await Func00();
	std::cout << "Task 1 exec" << "\n";

	co_return aa;
}

TaskVoid Func2()
{
	std::cout << "Task 2 exec " << "\n";
	auto aa = co_await Func1();
	// co_await Func1();
	std::cout << "Task 2 after " << "\n";
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

export void ApiAuth(Server::CVPtr server)
{

	WebProxyHelper::CVPtr webProxyHelper = server->GetComponent<WebProxyHelper>(EMComponentType::WebProxy);

	webProxyHelper->service->POST("/Auth/User/LoginToken", [server](hv::HttpRequestPtr req, hv::HttpResponseWriterPtr writer) -> TaskVoid
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
				co_return;
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
				co_return;
			}

			AuthServerHelper::CVPtr dnServer = server->GetSelf<AuthServerHelper>();

			try
			{
				
				auto transaction = dnServer->GetRdbProxy()->GetTransaction(EMSqlDbNameEnum::Account);

				if(!transaction)
				{
					
					errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
					errData["Message"] = "RDB Disconnect!";
					MSGSET(errData.dump());
					writer->End();
					co_return;
				}

				DbSqlHelper<GDb::Account> accounts(transaction.get(), dnServer->GetWorld());

				accounts
					.InitEntity(accInfo)
					.SelectAll()
					.SelectCond<GDb::Account::kAuthNameFieldNumber>("=", "")
					.SelectCond<GDb::Account::kAuthStringFieldNumber>("=", " AND ")
					.Limit(2)
					.Commit();

				const auto& results = accounts.GetResult();

				if (results.size() != 1)
				{
					
					errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
					errData["Message"] = "not Account!";
					MSGSET(errData.dump());
					writer->End();
					co_return;
				}

				accInfo.Swap(results[0].get());

				transaction->commit();
			}
			catch (const std::exception& e)
			{
				
				LoggerPrint::Log(server->GetWorld(), ELogLevel_Debug, "{}", e.what());
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

			ClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();

	
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

			AuthServerHelper::CVPtr dnServer = server->GetSelf<AuthServerHelper>();

			try
			{
				auto transaction = dnServer->GetRdbProxy()->GetTransaction(EMSqlDbNameEnum::Account);

				if(!transaction)
				{
					
					errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
					errData["Message"] = "RDB Disconnect!";
					MSGSET(errData.dump());
					writer->End();
					return;
				}
				
				DbSqlHelper<GDb::Account> accounts(transaction.get(), dnServer->GetWorld());

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
				LoggerPrint::Log(server->GetWorld(), ELogLevel_Debug, "{}", e.what());
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
				auto transaction = dnServer->GetRdbProxy()->GetTransaction(EMSqlDbNameEnum::Account, false);

				if(!transaction)
				{
					
					errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
					errData["Message"] = "RDB Disconnect!";
					MSGSET(errData.dump());
					writer->End();
					return;
				}

				DbSqlHelper<GDb::Account> accounts(transaction.get(), dnServer->GetWorld());

				accounts.InitEntity(accInfo).Insert().Commit();

				transaction->commit();

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
				LoggerPrint::Log(server->GetWorld(), ELogLevel_Debug, "{}", e.what());
				errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["Message"] = "Regist Error!!";
				MSGSET(errData.dump());
			}

			writer->End();
		});

	webProxyHelper->service->POST("/Auth/Test/User", [server](hv::HttpRequestPtr req, hv::HttpResponseWriterPtr writer)-> TaskVoid
		{
			// std::cout << "begin" << "\n";
			// co_await Func4();
			// writer->End();
			// std::cout << "end" << "\n";

			AuthServerHelper::CVPtr dnServer = server->GetSelf<AuthServerHelper>();
			World::CVPtr world = dnServer->GetWorld();

			try
			{

				if(auto transaction = dnServer->GetMdbProxy()->GetTransaction())
				{
					auto result = transaction->hset("hello", "world", "1").exec().get<bool>(0);
					LoggerPrint::Log(world, ELogLevel_Debug, "{}", result);
					
				}

				if(auto transaction = dnServer->GetMdbProxy()->GetTransaction())
				{
					auto result = transaction->hget("hello", "world").exec().get<sw::redis::OptionalString>(0);
					if(result)
					{
						LoggerPrint::Log(world, ELogLevel_Debug, "{}", *result);
					}
				}
			}
			catch(const std::exception& e)
			{
				LoggerPrint::Log(world, ELogLevel_Debug, "1232:{}", e.what());
			}

			writer->End();

			co_return;
		});
}