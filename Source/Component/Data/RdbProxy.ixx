export module RdbProxy;

import ThirdParty.Libpqxx;
import Logger;
import ECSW;
import std.compat;
import StrUtils;

export enum class EMSqlDbNameEnum : uint16_t
{
	Account,
	Nightmare,
};

export class RdbProxy : public Component
{
protected:
	friend class System;
	friend class UniversalMemoryPool;
	RdbProxy(System::WPtr system):Component(system)
	{
		eComponentType = EMComponentType::RdbProxy;

		pLogger = GetOwner()->GetWorld()->GetSystemW<LoggerPrint>(EMSystemType::LoggerPrint);
	}

public:
	using Ptr = std::shared_ptr<RdbProxy>;
	using CVPtr = const Ptr&;

	virtual void Dispose() override
	{
		Component::Dispose();

		pRdbProxys.clear();
	}

	LoggerPrint::Ptr GetLogger(){ return pLogger.expired() ? nullptr : pLogger.lock(); }

	virtual bool Awake() override
	{
		GetOwner()->AddEvent(EMEventType::ServerStart, GetSelfW<RdbProxy>(), &RdbProxy::InitDatabase);
		return true;
	}

	void InitDatabase()
	{
		World::CVPtr world = GetOwner()->GetWorld();

		std::string* value = world->LaunchParam("connection");
		pqxx::connection check(*value);
		pqxx::nontransaction checkTxn(check);

		std::string* names = world->LaunchParam("dbnames");
		
		std::vector<std::string> dbNames = StrSplit(*names, ",");
		
		for (std::string& dbName : dbNames)
		{
			EnumName<EMSqlDbNameEnum>(dbName); // check vaild = assert
			
			if (!checkTxn.query_value<bool>(std::format("SELECT EXISTS (SELECT 1 FROM pg_database WHERE datname = '{}');", dbName)))
			{
				checkTxn.exec(std::format("CREATE DATABASE \"{}\";", dbName));
				GetLogger()->Record(ELogLevel_Debug, "Create Database:{}", dbName);
			}

			uint16_t key = (uint16_t)EnumName<EMSqlDbNameEnum>(dbName);
			std::string connectStr = std::format("{} dbname = {}", *value, dbName);

			// auto connection = std::make_shared<pqxx::connection>(connectStr);
			auto connection = MemPool->Allocate<pqxx::connection, const std::string&>(connectStr);

			pRdbProxys.emplace(key, std::move(connection));
		}
	
	}
	

public:
	~RdbProxy() = default;

protected:
	std::unordered_map<uint16_t, std::shared_ptr<pqxx::connection>> pRdbProxys;

	LoggerPrint::WPtr pLogger;
};
