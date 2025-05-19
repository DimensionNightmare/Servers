module;
export module ApiManager;

import :ApiAuth;
import ThirdParty.Libhv;
import AuthServerHelper;
import DNServer;

export void ApiInit(DNServer::WPtr server, hv::HttpService* service)
{
	service->preprocessor = [server](const hv::HttpContextPtr& ctx) -> int
		{
			static bool pass = 0;

			if (ctx->request->path.contains("/Test/"))
			{
				return pass;
			}

			DNServer::Ptr serverTemp = server.lock();
			if(!serverTemp) { return !pass; }


			nlohmann::json errData;

			AuthServerHelper::Ptr dnServer = serverTemp->GetSelf<AuthServerHelper>();
			if (dnServer->GetClientProxy()->GetRegistState() != EMRegistState::Registed)
			{
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Server Disconnect!";
				ctx->response->SetBody(errData.dump());
				return !pass;
			}

			return pass;
		};

	ApiAuth(server, service);
}