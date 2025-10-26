export module RdbProxy;

import ThirdParty.Libpqxx;
import Logger;
import ECSW;
import std.compat;
import StrUtils;

export enum class EMSqlDbNameEnum : uint16_t
{
	None = 0,
	Account,
	Nightmare,
};

export class RdbProxy : public Component
{
protected:
	friend class UniversalMemoryPool;
	RdbProxy(System::WPtr system):Component(system)
	{
		eComponentType = EMComponentType::RdbProxy;
	}

public:
	using Ptr = std::shared_ptr<RdbProxy>;

	virtual void Dispose() override
	{
		Component::Dispose();

		pRdbProxys.clear();
	}

	virtual bool Awake() override
	{
		GetOwner()->AddEvent(EMEventType::ServerStart, GetSelfW<RdbProxy>(), &RdbProxy::InitDatabase);
		return true;
	}

	void InitDatabase()
	{
		World::Ptr world = GetWorld();

		std::string* value = world->LaunchParam("connection");
		pqxx::connection check(*value);
		pqxx::nontransaction checkTxn(check);

		std::string* names = world->LaunchParam("dbnames");
		
		std::vector<std::string> dbNames = StrSplit(*names, ",");
		
		for (std::string& dbName : dbNames)
		{
			EMSqlDbNameEnum key = EnumName<EMSqlDbNameEnum>(dbName); // check vaild = assert
			
			if (!checkTxn.query_value<bool>(std::format("SELECT EXISTS (SELECT 1 FROM pg_database WHERE datname = '{}');", dbName)))
			{
				checkTxn.exec(std::format("CREATE DATABASE \"{}\";", dbName));
				LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Create Database:{}", dbName);
			}

			std::string connectStr = std::format("{} dbname = {}", *value, dbName);

			auto connection = P_InstanceHolder->GetMemPool().Allocate<pqxx::connection>(connectStr);

			pRdbProxys.emplace(key, std::move(connection));
		}
	
	}
	

public:
	virtual ~RdbProxy()
	{
		
	}

protected:
	std::unordered_map<EMSqlDbNameEnum, std::shared_ptr<pqxx::connection>> pRdbProxys;
};
