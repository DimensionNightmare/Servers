module;
#include "StdMacro.h"
export module AuthServerHelper;

export import AuthServer;
export import DNClientProxyHelper;
export import DNWebProxyHelper;
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
			string* value = GetLuanchConfigParam("connection");

			string* dbName = GetLuanchConfigParam("dbname");

			pSqlProxy = make_unique<pq_connection>(format("{} dbname = {}", *value, *dbName));

		}
		catch (const exception& e)
		{
			DNPrint(0, LoggerLevel::Debug, "%s", e.what());
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
