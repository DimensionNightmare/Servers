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
		if (string* names = GetLuanchConfigParam("dbnames"))
		{
			size_t start = 0;
			size_t end = names->find(",");
			string name;
			while (end != string::npos)
			{
				name = names->substr(start, end - start);
				EnumName<SqlDbNameEnum>(name);

				dbNames.push_back(name);
				start = end + 1;
				end = names->find(",", start);
			}

			name = names->substr(start);
			EnumName<SqlDbNameEnum>(name);
			dbNames.push_back(name);

			for (string& dbName : dbNames)
			{
				if (!checkTxn.query_value<bool>(format("SELECT EXISTS (SELECT 1 FROM pg_database WHERE datname = '{}');", dbName)))
				{
					checkTxn.exec(format("CREATE DATABASE \"{}\";", dbName));
					DNPrint(0, LoggerLevel::Debug, "Create Database:%s", dbName.c_str());
				}

				uint16_t key = (uint16_t)EnumName<SqlDbNameEnum>(dbName);
				string connectStr = format("{} dbname = {}", *value, dbName);
				pSqlProxys[key] = make_unique<pqxx::connection>(connectStr);
			}
		}
		else
		{
			return false;
		}

		// check over connect other
		uint16_t dbNameKey = (uint16_t)SqlDbNameEnum::Account;
		if (pSqlProxys.count(dbNameKey))
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

		dbNameKey = (uint16_t)SqlDbNameEnum::Nightmare;
		if (pSqlProxys.count(dbNameKey))
		{
			pqxx::work txn(*pSqlProxys[dbNameKey]);

			DbSqlHelper<Player> playerInfo(&txn);
			if (!playerInfo.IsExist())
			{
				DNPrint(0, LoggerLevel::Debug, "Create Table:Player");
				playerInfo.InitTable().Commit();
			}

			// Player player;
			// player.set_account_id(11);
			// PropertyEntity* propertyEntity = player.mutable_property_entity();
			// propertyEntity->set_hp_max(1);
			// propertyEntity->set_mp_max(1);
			// propertyEntity->set_attack(1);
			// propertyEntity->set_defense(1);
			// playerInfo.Insert(player).Commit();

			playerInfo.SelectAll().Limit(1).Commit();

			Player player = playerInfo.Result()[0];
			player.mutable_property_entity()->set_hp_max(2000);
			playerInfo
				DBUpdate(player, property_entity)
				DBUpdateCond(player, account_id, "=", "")
				.Commit();

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
