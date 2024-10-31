module;
#include "hv/json.hpp"
#include "StdMacro.h"
export module ApiManager;

import AuthServerHelper;
import :ApiAuth;
import ThirdParty.Libhv;

export void ApiInit(HttpService* service)
{
	service->preprocessor = [](const HttpContextPtr& ctx) -> int
		{
			static bool pass = 0;

			if (ctx->request->path.contains("/Test/"))
			{
				return pass;
			}

			AuthServerHelper* authServer = GetAuthServer();

			nlohmann::json errData;

			if (authServer->GetCSock()->EMRegistState() != EMRegistState::Registed)
			{
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Server Disconnect!";
				ctx->response->SetBody(errData.dump());
				return !pass;
			}

			return pass;
		};

	ApiAuth(service);
}