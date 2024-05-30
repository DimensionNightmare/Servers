module;
#include <format>
#include <cstdint>
#include "pqxx/connection"
#include "pqxx/transaction"
#include "pqxx/nontransaction"

#include "StdMacro.h"
#include "GDef/GDef.pb.h"
export module DatabaseServerHelper;

export import DatabaseServer;
export import DNClientProxyHelper;
export import ServerEntityManagerHelper;
import Logger;
import Config.Server;
import StrUtils;
import DbUtils;

using namespace std;
using namespace GDb;

enum class SqlDbNameEnum : uint16_t
{
	Account,
	Nightmare,
};

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
	try
	{
		//"postgresql://root@localhost"
		string* value = GetLuanchConfigParam("connection");
		pqxx::connection check(*value);
		pqxx::nontransaction checkTxn(check);

		list<string> dbNames;
		if(string* value = GetLuanchConfigParam("dbnames"))
		{
			size_t start = 0;
			size_t end = value->find(",");

			while (end != string::npos)
			{
				dbNames.push_back(value->substr(start, end - start));
				start = end;
				end = value->find(",", start);
			}
			
			for(string& dbName : dbNames)
			{
				if (!checkTxn.query_value<bool>(format("SELECT EXISTS (SELECT 1 FROM pg_database WHERE datname = '{}');", dbName)))
				{
					checkTxn.exec(format("CREATE DATABASE \"{}\";", dbName));
					DNPrint(0, LoggerLevel::Debug, "Create Database:%s", dbName.c_str());
				}

				uint16_t key = (uint16_t)EnumName<SqlDbNameEnum>(dbName);
				pSqlProxys[key] = make_unique<pqxx::connection>(format("{} dbname = {}", *value, dbName));
			}
		}
		else
		{
			return false;
		}

		// check over connect other
		uint16_t dbNameKey = (uint16_t)SqlDbNameEnum::Account;
		if(pSqlProxys.count(dbNameKey))
		{
			pqxx::work txn(*pSqlProxys[dbNameKey]);

			DbSqlHelper<Account> accountInfo(&txn);
			if (!accountInfo.IsExist())
			{
				DNPrint(0, LoggerLevel::Debug, "Create Table:Account");
				accountInfo.InitTable().Commit();
			}
			txn.commit();
		}
		
	}
	catch (const exception& e)
	{
		DNPrint(0, LoggerLevel::Debug, "%s", e.what());
		return false;
	}

	return true;
}
