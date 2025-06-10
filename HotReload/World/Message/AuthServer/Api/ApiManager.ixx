module;
export module ApiManager;

import :ApiAuth;
import DNClientProxyHelper;

export void ApiInit(DNServer::CVPtr dnServer)
{
	DNServer::WPtr server = dnServer->GetSelfW<DNServer>();

	DNWebProxyHelper::Ptr webProxyHelper = dnServer->GetComponent<DNWebProxyHelper>(EMComponentType::DNWebProxy);

	webProxyHelper->service->preprocessor = [server](const hv::HttpContextPtr& ctx) -> int
		{
			static bool pass = 0;

			if (ctx->request->path.contains("/Test/"))
			{
				return pass;
			}

			DNServer::Ptr dnServer = server.lock();
			if(!dnServer) { return !pass; }


			nlohmann::json errData;

			DNClientProxyHelper::Ptr clientSock = dnServer->GetComponent<DNClientProxyHelper>(EMComponentType::DNClientProxy);
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