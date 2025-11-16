export module RdbProxy;

import ThirdParty.Libpqxx;
import Logger;
import ECSW;
import std.compat;
import StrUtils;
import Timer;
import Server;

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
		GetWorld()->AddEvent<&RdbProxy::InitDatabase>(EMEventType::ServerStart, GetSelfW<RdbProxy>());
		return true;
	}

	void InitDatabase()
	{
		World::Ptr world = GetWorld();

		std::string* param = world->GetParam("rdbConnection");
		if(!param)
		{
			return;
		}

		std::shared_ptr<pqxx::connection> connection;

		try
		{
			connection = P_InstanceHolder->GetMemPool().Allocate<pqxx::connection>(*param);
		}
		catch(pqxx::broken_connection& e)
		{
			LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Can Connect Database:{}, retest", *param);
			// 重试 retest
			Timer::Ptr timer = GetWorld()->GetSystem<Timer>(EMSystemType::Timer);

			timer->SetTimeout(3000, [this](size_t)
			{
				InitDatabase();
			});
			return;
		}
		catch(std::exception& e)
		{
			LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Can Connect Database:{}, no retest", e.what());
			return;
		}

		pqxx::nontransaction checkTxn(*connection);

		std::string* names = world->GetParam("dbnames");
		if(!names)
		{
			return;
		}
		
		for (const auto& dbName : StrSplit(*names, ","))
		{
			EMSqlDbNameEnum key = EnumName<EMSqlDbNameEnum>(dbName); // check vaild = assert
			
			if (!checkTxn.query_value<bool>(std::format("SELECT EXISTS (SELECT 1 FROM pg_database WHERE datname = '{}');", dbName)))
			{
				checkTxn.exec(std::format("CREATE DATABASE \"{}\";", dbName));
				LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Create Database:{}", dbName);
			}

			std::string connectStr = std::format("{} dbname = {}", *param, dbName);

			auto connection = P_InstanceHolder->GetMemPool().Allocate<pqxx::connection>(connectStr);

			pRdbProxys.emplace(key, std::move(connection));
		}

		auto server = GetOwner<Server>();
		if(server && server->GetServerType() == EMServerType::DatabaseServer)
		{
			GetWorld()->Broadcast(EMEventType::InitedRdbConnection);
		}
	
	}
	

public:
	virtual ~RdbProxy()
	{
		
	}

protected:
	std::unordered_map<EMSqlDbNameEnum, std::shared_ptr<pqxx::connection>> pRdbProxys;
};
