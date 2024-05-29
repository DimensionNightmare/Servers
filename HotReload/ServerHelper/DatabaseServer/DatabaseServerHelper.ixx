module;
#include <format>
#include <cstdint>
#include "pqxx/connection"
#include "pqxx/transaction"
#include "pqxx/nontransaction"

#include "StdMacro.h"
export module DatabaseServerHelper;

export import DatabaseServer;
export import DNClientProxyHelper;
export import ServerEntityManagerHelper;
import Logger;
import Config.Server;

using namespace std;

export class DatabaseServerHelper : public DatabaseServer
{
private:
	DatabaseServerHelper() = delete;;
public:

	DNClientProxyHelper* GetCSock() { return nullptr; }
	ServerEntityManagerHelper* GetServerEntityManager() { return nullptr; }

	bool InitDatabase();

	string& GetCtlIp() { return sCtlIp; }
	uint16_t& GetCtlPort() { return iCtlPort; }
};

static DatabaseServerHelper* PDatabaseServerHelper = nullptr;

export void SetDatabaseServer(DatabaseServer* server)
{
	PDatabaseServerHelper = static_cast<DatabaseServerHelper*>(server);
	assert(PDatabaseServerHelper != nullptr);
}

export DatabaseServerHelper* GetDatabaseServer()
{
	return PDatabaseServerHelper;
}

bool DatabaseServerHelper::InitDatabase()
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
	}
	catch (const exception& e)
	{
		DNPrint(0, LoggerLevel::Debug, "%s", e.what());
		return false;
	}

	return true;
}
