module;
#include "hv/HttpServer.h"
export module ApiManager;

import AuthServerHelper;
import :ApiAuth;

using namespace hv;

export void ApiInit(HttpService* service)
{
	service->preprocessor = [](HttpRequest* req, HttpResponse* resp) -> int
		{
			if (req->path.contains("/Test/"))
			{
				return HTTP_STATUS_NEXT;
			}

			AuthServerHelper* authServer = GetAuthServer();

			Json errData;

			if (authServer->GetCSock()->RegistState() != RegistState::Registed)
			{
				errData["code"] = HTTP_STATUS_BAD_REQUEST;
				errData["message"] = "Server Disconnect!";
				resp->SetBody(errData.dump());
				return !HTTP_STATUS_NEXT;
			}

			return HTTP_STATUS_NEXT;
		};

	ApiAuth(service);
}