export module ApiManager;

import :ApiAuth;
import ClientProxyHelper;
import :ApiDevelopment;

export void ApiInit(Server::CVPtr dnServer)
{
	Server::WPtr server = dnServer;

	WebProxyHelper::CVPtr webProxyHelper = dnServer->GetComponent<WebProxyHelper>(EMComponentType::WebProxy);

	webProxyHelper->service->preprocessor = [server](const hv::HttpContextPtr& ctx) -> int
		{
			static bool pass = 0;

			if (ctx->request->path.contains("/Test/"))
			{
				return pass;
			}

			Server::CVPtr dnServer = server.lock();
			if(!dnServer || dnServer->IsDisposed()) { return !pass; }


			nlohmann::json errData;

			ClientProxyHelper::CVPtr clientProxy = dnServer->GetComponent<ClientProxyHelper>(EMComponentType::ClientProxy);
			if (clientProxy->GetRegistState() != EMRegistState::Registed)
			{
				errData["Code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["Message"] = "Server Disconnect!";
				ctx->response->SetBody(errData.dump());
				return !pass;
			}

			return pass;
		};

	ApiAuth(dnServer);
	ApiDevelopment(dnServer);
}