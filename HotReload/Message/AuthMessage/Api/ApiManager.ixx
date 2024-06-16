module;
#include <functional>

#include "hv/json.hpp"
export module ApiManager;

import AuthServerHelper;
import :ApiAuth;
import ThirdParty.Libhv;

using namespace std;

export void ApiInit(HttpService* service)
{
	service->preprocessor = [](const HttpContextPtr& ctx) -> int
		{
			static int success = 0;

			if (ctx->request->path.contains("/Test/"))
			{
				return success;
			}

			AuthServerHelper* authServer = GetAuthServer();

			nlohmann::json errData;

			if (authServer->GetCSock()->RegistState() != RegistState::Registed)
			{
				errData["code"] = http_status::HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Server Disconnect!";
				ctx->response->SetBody(errData.dump());
				return !success;
			}

			return success;
		};

	ApiAuth(service);
}