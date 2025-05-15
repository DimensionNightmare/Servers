module;
export module RdbProxy;

import ECSW;
import ThirdParty.Libpqxx;
import Logger;

export class RdbProxy : public Component
{
protected:
	friend class System;
	RdbProxy(System::WPtr system):Component(system)
	{
		eComponentType = EMComponentType::RdbProxy;

		pLogger = GetOwner()->GetWorld()->GetSystemW<LoggerPrint>(EMSystemType::LoggerPrint);
	}

	bool Awake()
	{
		
		World::Ptr pWorld = GetOwner()->GetWorld();

		try
		{
			//"postgresql://root@localhost"
			std::string* value = pWorld->LaunchParam("connection");

			std::string* dbName = pWorld->LaunchParam("dbname");

			pMdbProxy = std::make_unique<pqxx::connection>(std::format("{} dbname = {}", *value, *dbName));

		}
		catch (const std::exception& e)
		{
			GetLogger()->Record(ELogLevel_Debug, "{}", e.what());
			return false;
		}

		
		return true;
	}

	virtual void Dispose() override
	{
		Component::Dispose();
	}

	LoggerPrint::Ptr GetLogger(){ return pLogger.expired() ? nullptr : pLogger.lock(); }

public:
	~RdbProxy() = default;

protected:
	std::unique_ptr<pqxx::connection> pMdbProxy;

	LoggerPrint::WPtr pLogger;
};
