module;
#include <format>
#include "pqxx/connection"
#include "pqxx/transaction"
#include "pqxx/nontransaction"

#include "StdMacro.h"
#include "GDef/GDef.pb.h"
export module AuthServerHelper;

export import AuthServer;
export import DNClientProxyHelper;
export import DNWebProxyHelper;
import DNDbObj;
import Logger;
import Config.Server;

using namespace std;
using namespace GDb;

export class AuthServerHelper : public AuthServer
{
private:
	AuthServerHelper() = delete;
public:
	DNClientProxyHelper* GetCSock() { return nullptr; }
	DNWebProxyHelper* GetSSock() { return nullptr; }
	bool InitDatabase();
};

static AuthServerHelper* PAuthServerHelper = nullptr;

export void SetAuthServer(AuthServer* server)
{
	PAuthServerHelper = static_cast<AuthServerHelper*>(server);
}

export AuthServerHelper* GetAuthServer()
{
	return PAuthServerHelper;
}

bool AuthServerHelper::InitDatabase()
{
	if (pSqlProxy)
	{
		return true;
	}
	
	try
	{
		//"postgresql://root@localhost"
		string* value = GetLuanchConfigParam("connection");
		pqxx::connection check(*value);
		pqxx::nontransaction checkTxn(check);

		string dbName;
		if(string* value = GetLuanchConfigParam("dbname"))
		{
			dbName = *value;
		}
		else
		{
			return false;
		}

		if (!checkTxn.query_value<bool>(format("SELECT EXISTS (SELECT 1 FROM pg_database WHERE datname = '{}');", dbName)))
		{
			checkTxn.exec(format("CREATE DATABASE \"{}\";", dbName));
		}
		
		// check over connect other
		pSqlProxy = make_unique<pqxx::connection>(format("{} dbname = {}", *value, dbName));
		pqxx::work txn(*pSqlProxy);

		DNDbObj<Account> accountInfo(&txn);
		if (!accountInfo.IsExist())
		{
			accountInfo.InitTable().Commit();
		}

		txn.commit();
	}
	catch (const exception& e)
	{
		DNPrint(0, LoggerLevel::Debug, "%s", e.what());
		return false;
	}

	return true;
}
