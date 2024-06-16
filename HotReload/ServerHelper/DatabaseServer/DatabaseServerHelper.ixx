module;
#include <format>
#include <cstdint>
#include <list>

#include "StdMacro.h"
export module DatabaseServerHelper;

export import DatabaseServer;
export import DNClientProxyHelper;
export import ServerEntityManagerHelper;
import Logger;
import Config.Server;
import StrUtils;
import DbUtils;
import ThirdParty.PbGen;
import ThirdParty.Libpqxx;

export enum class SqlDbNameEnum : uint16_t
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

	connection* GetSqlProxy(SqlDbNameEnum nameEnum);
};

static DatabaseServerHelper* PDatabaseServerHelper = nullptr;

export void SetDatabaseServer(DatabaseServer* server)
{
	PDatabaseServerHelper = static_cast<DatabaseServerHelper*>(server);
	ASSERT(PDatabaseServerHelper != nullptr);
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
		connection check(*value);
		nontransaction checkTxn(check);

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

				dbNames.emplace_back(name);
				start = end + 1;
				end = names->find(",", start);
			}

			name = names->substr(start);
			EnumName<SqlDbNameEnum>(name);
			dbNames.emplace_back(name);

			for (string& dbName : dbNames)
			{
				if (!checkTxn.query_value<bool>(format("SELECT EXISTS (SELECT 1 FROM pg_database WHERE datname = '{}');", dbName)))
				{
					checkTxn.exec(format("CREATE DATABASE \"{}\";", dbName));
					DNPrint(0, LoggerLevel::Debug, "Create Database:%s", dbName.c_str());
				}

				uint16_t key = (uint16_t)EnumName<SqlDbNameEnum>(dbName);
				string connectStr = format("{} dbname = {}", *value, dbName);
				pSqlProxys[key] = make_unique<connection>(connectStr);
			}
		}
		else
		{

			return false;
		}

		unordered_map<SqlDbNameEnum, vector<Message*> > registTable = {
			{
				SqlDbNameEnum::Account,
				{
					(Message*)Account::internal_default_instance(),
				}
			},
			{
				SqlDbNameEnum::Nightmare,
				{
					(Message*)Player::internal_default_instance(),
				}
			},
		};

		SingleTon kv;
		string schemaMd5;

		for (auto& [dbNameEnum, dbEntitys] : registTable)
		{
			uint16_t index = (uint16_t)dbNameEnum;
			if (pSqlProxys.contains(index))
			{
				pq_work txn(*pSqlProxys[index]);
				DbSqlHelper<SingleTon> singleTon(&txn);
				singleTon.InitEntity(kv);

				if (!singleTon.IsExist())
				{
					DNPrint(0, LoggerLevel::Debug, "Create Table:SingleTon");
					singleTon.CreateTable().Commit();
				}

				for (Message* dbEntity : dbEntitys)
				{
					DbSqlHelper<Message> helper(&txn, dbEntity);

					const string& tableName = helper.GetName();
					kv.set_key(format("{}_Schema", tableName));
					schemaMd5 = helper.GetTableSchemaMd5();

					if (!helper.IsExist())
					{

						DNPrint(0, LoggerLevel::Debug, "Create Table:%s", tableName.c_str());
						helper.CreateTable().Commit();

						kv.set_value(schemaMd5);
						singleTon.Insert().Commit();
						continue;
					}

					singleTon
						DBSelectByKey(kv, key)
						.Commit();

					if (!singleTon.IsSuccess() || !singleTon.Result().size())
					{
						continue;
					}

					kv = *singleTon.Result()[0];

					if (schemaMd5 != kv.value())
					{
						// cout << helper.UpdateTable().GetBuildSqlStatement() << endl;
						helper.UpdateTable().Commit();


						kv.set_value(schemaMd5);
						singleTon.UpdateByKey("key").Commit();
					}
				}

				txn.commit();
			}
		}

	}
	catch (const exception& e)
	{
		DNPrint(0, LoggerLevel::Debug, "%s", e.what());
		return false;
	}

	return true;
}

connection* DatabaseServerHelper::GetSqlProxy(SqlDbNameEnum nameEnum)
{
	uint16_t dbNameKey = (uint16_t)nameEnum;
	if (pSqlProxys.contains(dbNameKey))
	{
		return &*pSqlProxys[dbNameKey];
	}
	return nullptr;
}
