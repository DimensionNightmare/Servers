module;
export module ApiManager;

import :ApiAuth;
import ClientProxyHelper;

export void ApiInit(Server::CVPtr dnServer)
{
	Server::WPtr server = dnServer->GetSelfW<Server>();

	WebProxyHelper::Ptr webProxyHelper = dnServer->GetComponent<WebProxyHelper>(EMComponentType::WebProxy);

	webProxyHelper->service->preprocessor = [server](const hv::HttpContextPtr& ctx) -> int
		{
			static bool pass = 0;

			if (ctx->request->path.contains("/Test/"))
			{
				return pass;
			}

			Server::Ptr dnServer = server.lock();
			if(!dnServer) { return !pass; }


			nlohmann::json errData;

			ClientProxyHelper::Ptr clientSock = dnServer->GetComponent<ClientProxyHelper>(EMComponentType::ClientProxy);
			if (clientSock->GetRegistState() != EMRegistState::Registed)
			{
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Server Disconnect!";
				ctx->response->SetBody(errData.dump());
				return !pass;
			}

			return pass;
		};

	ApiAuth(dnServer);
}