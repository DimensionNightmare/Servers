module;
#include "StdMacro.h"
export module AuthServerHelper;

import AuthServer;
import DNClientProxyHelper;
import DNWebProxyHelper;
import DbUtils;
import Logger;
import Config.Server;
import ThirdParty.Libpqxx;

export class AuthServerHelper : public AuthServer
{

private:

	AuthServerHelper() = delete;
public:

	DNClientProxyHelper* GetCSock() { return nullptr; }

	DNWebProxyHelper* GetSSock() { return nullptr; }

	bool InitDatabase()
	{
		if (pSqlProxy)
		{
			return true;
		}

		try
		{
			//"postgresql://root@localhost"
			std::string* value = GetLuanchConfigParam("connection");

			std::string* dbName = GetLuanchConfigParam("dbname");

			pSqlProxy = std::make_unique<pq_connection>(std::format("{} dbname = {}", *value, *dbName));

		}
		catch (const std::exception& e)
		{
			DNPrint(0, EMLoggerLevel::Debug, "%s", e.what());
			return false;
		}

		return true;
	}
};

static AuthServerHelper* PAuthServerHelper = nullptr;

export void SetAuthServer(AuthServer* server)
{
	PAuthServerHelper = static_cast<AuthServerHelper*>(server);
	ASSERT(PAuthServerHelper != nullptr)
}

export AuthServerHelper* GetAuthServer()
{
	return PAuthServerHelper;
}
