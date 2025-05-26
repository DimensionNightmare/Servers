module;
export module DatabaseServerHelper;

import DNClientProxyHelper;
import ServerEntityManagerHelper;
import DbUtils;
import RdbProxy;
import StrUtils;
import DNServer;

export class DatabaseServerHelper : public DNServer
{

private:

	DatabaseServerHelper() = delete;
	~DatabaseServerHelper() = default;

	DatabaseServerHelper(const DatabaseServerHelper&) = delete;
	// void operator=(const DatabaseServerHelper&) = delete;

	DatabaseServerHelper(DatabaseServerHelper&&) = delete;
	DatabaseServerHelper& operator=(DatabaseServerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<DatabaseServerHelper>;

	DNClientProxyHelper::Ptr GetClientProxy() { return GetComponent<DNClientProxyHelper>(EMComponentType::DNClientProxy); }

	ServerEntityManagerHelper::Ptr GetServerEntityManager() { return GetComponent<ServerEntityManagerHelper>(EMComponentType::ServerEntityManager); }

	RdbProxy::Ptr GetRdbProxy(){ return GetComponent<RdbProxy>(EMComponentType::RdbProxy); }

	bool InitDatabase()
	{
		if(RdbProxy::Ptr proxy = GetRdbProxy())
		{
			try
			{
				World::Ptr pWorld = GetWorld();

				
				std::string* value = pWorld->LaunchParam("connection");
				pqxx::connection check(*value);
				pqxx::nontransaction checkTxn(check);

				if (std::string* names = pWorld->LaunchParam("dbnames"))
				{
					std::vector<std::string> dbNames = StrSplit(*names, ",");
					
					for (std::string& dbName : dbNames)
					{
						try
						{
							EnumName<EMSqlDbNameEnum>(dbName); // check vaild
						}
						catch(...)
						{
							return false;
						}

						if (!checkTxn.query_value<bool>(std::format("SELECT EXISTS (SELECT 1 FROM pg_database WHERE datname = '{}');", dbName)))
						{
							checkTxn.exec(std::format("CREATE DATABASE \"{}\";", dbName));
							GetLogger()->Record(ELogLevel_Debug, "Create Database:{}", dbName);
						}

						uint16_t key = (uint16_t)EnumName<EMSqlDbNameEnum>(dbName);
						std::string connectStr = std::format("{} dbname = {}", *value, dbName);

						auto connection = std::make_shared<pqxx::connection>(connectStr);

						proxy->AddConnection(key, std::move(connection));
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
							(Message*)GDb::Account::internal_default_instance(),
						}
					},
					{
						EMSqlDbNameEnum::Nightmare,
						{
							(Message*)GDb::Player::internal_default_instance(),
						}
					},
				};

				GDb::SingleTon kv;
				std::string schemaMd5;

				for (auto& [dbNameEnum, dbEntitys] : registTable)
				{
					if (auto connection = proxy->GetConnection((uint16_t)dbNameEnum))
					{
						pqxx::work txn(*connection);
						DbSqlHelper<GDb::SingleTon> singleTon(&txn);
						singleTon.InitEntity(kv);

						if (!singleTon.IsExist())
						{
							GetLogger()->Record(ELogLevel_Debug, "Create Table:SingleTon");
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

								GetLogger()->Record(ELogLevel_Debug, "Create Table:{}", tableName);
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
								GetLogger()->Record(ELogLevel_Debug, "not match md5:\n{}\n{}", schemaMd5, kv.value());
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
				GetLogger()->Record(ELogLevel_Debug, e.what());
				return false;
			}

			return true;
		}

		return false;
	}
};
