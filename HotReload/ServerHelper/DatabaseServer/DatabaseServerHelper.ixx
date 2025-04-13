module;
export module DatabaseServerHelper;

import DatabaseServer;
import DNClientProxyHelper;
import ServerEntityManagerHelper;
import Logger;
import Config.Server;
import StrUtils;
import DbUtils;
import ThirdParty.PbGen;
import ThirdParty.Libpqxx;
import std.compat;

export enum class EMSqlDbNameEnum : uint16_t
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

	bool InitDatabase()
	{
		try
		{
			//"postgresql://root@localhost"
			std::string* value = LaunchConfig::GetParam("connection");
			pq_connection check(*value);
			nontransaction checkTxn(check);

			std::list<std::string> dbNames;
			if (std::string* names = LaunchConfig::GetParam("dbnames"))
			{
				size_t start = 0;
				size_t end = names->find(",");
				std::string name;
				while (end != std::string::npos)
				{
					name = names->substr(start, end - start);
					EnumName<EMSqlDbNameEnum>(name);

					dbNames.emplace_back(name);
					start = end + 1;
					end = names->find(",", start);
				}

				name = names->substr(start);
				EnumName<EMSqlDbNameEnum>(name);
				dbNames.emplace_back(name);

				for (std::string& dbName : dbNames)
				{
					if (!checkTxn.query_value<bool>(std::format("SELECT EXISTS (SELECT 1 FROM pg_database WHERE datname = '{}');", dbName)))
					{
						checkTxn.exec(std::format("CREATE DATABASE \"{}\";", dbName));
						LoggerPrint()(ELogLevel_Debug, "Create Database:{}", dbName);
					}

					uint16_t key = (uint16_t)EnumName<EMSqlDbNameEnum>(dbName);
					std::string connectStr = std::format("{} dbname = {}", *value, dbName);
					pSqlProxys[key] = std::make_unique<pq_connection>(connectStr);
				}
			}
			else
			{

				return false;
			}

			std::unordered_map<EMSqlDbNameEnum, std::vector<Message*> > registTable = {
				{
					EMSqlDbNameEnum::Account,
					{
						(Message*)Account::internal_default_instance(),
					}
				},
				{
					EMSqlDbNameEnum::Nightmare,
					{
						(Message*)Player::internal_default_instance(),
					}
				},
			};

			SingleTon kv;
			std::string schemaMd5;

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
						LoggerPrint()(ELogLevel_Debug, "Create Table:SingleTon");
						singleTon.CreateTable().Commit();
					}

					for (Message* dbEntity : dbEntitys)
					{
						DbSqlHelper<Message> helper(&txn, dbEntity);

						const std::string& tableName = helper.GetName();
						kv.set_key(std::format("{}_Schema", tableName));
						schemaMd5 = helper.GetTableSchemaMd5();

						if (!helper.IsExist())
						{

							LoggerPrint()(ELogLevel_Debug, "Create Table:{}", tableName);
							helper.CreateTable().Commit();

							kv.set_value(schemaMd5);
							singleTon.Insert().Commit();
							continue;
						}

						// #define DBSelectOne(obj, name) .SelectOne(#name, [&obj]() { return obj.name(); })
						// #define DBSelectCond(obj, name, cond, splicing) .SelectCond(#name, cond, splicing, [&obj]() { return obj.name(); })
						// #define DBUpdate(obj, name) .Update(obj, #name, [&obj]() { return obj.name(); })
						// #define DBUpdateCond(obj, name, cond, splicing) .UpdateCond(#name, cond, splicing, [&obj]() { return obj.name(); })
						// #define DBDeleteCond(obj, name, cond, splicing) .DeleteCond(#name, cond, splicing, [&obj]() { return obj.name(); })

						// // key method
						// #define DBUpdateByKey(obj, name) .UpdateByKey(#name, [&obj]() { return obj.name(); })
						// #define DBSelectByKey(obj, name) .SelectByKey(#name, [&obj]() { return obj.name(); })

						#define DBSelectByKey(obj, name) .SelectByKey(#name, [&obj]() { return obj.name(); })

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
							LoggerPrint()(ELogLevel_Debug, "not match md5:\n{}\n{}", schemaMd5, kv.value());
							helper.UpdateTable().Commit();


							kv.set_value(schemaMd5);
							singleTon.UpdateByKey("key").Commit();
						}
					}

					txn.commit();
				}
			}

		}
		catch (const std::exception& e)
		{
			LoggerPrint()(ELogLevel_Debug, e.what());
			return false;
		}

		return true;
	}

	std::string& GetCtlIp() { return sCtlIp; }

	uint16_t& GetCtlPort() { return iCtlPort; }

	pq_connection* GetSqlProxy(EMSqlDbNameEnum nameEnum)
	{
		uint16_t dbNameKey = (uint16_t)nameEnum;
		if (pSqlProxys.contains(dbNameKey))
		{
			return &*pSqlProxys[dbNameKey];
		}
		return nullptr;
	}

};

static DatabaseServerHelper* PDatabaseServerHelper = nullptr;

export void SetDatabaseServer(DatabaseServer* server)
{
	PDatabaseServerHelper = static_cast<DatabaseServerHelper*>(server);
	if (!(PDatabaseServerHelper != nullptr)) {abort();}
}

export DatabaseServerHelper* GetDatabaseServer()
{
	return PDatabaseServerHelper;
}
