module;
export module RdbProxy;

import ThirdParty.Libpqxx;
import Logger;
import ECSW;
import std.compat;

export enum class EMSqlDbNameEnum : uint16_t
{
	Account,
	Nightmare,
};

export class RdbProxy : public Component
{
protected:
	friend class System;
	RdbProxy(System::WPtr system):Component(system)
	{
		eComponentType = EMComponentType::RdbProxy;

		pLogger = GetOwner()->GetWorld()->GetSystemW<LoggerPrint>(EMSystemType::LoggerPrint);
	}

public:
	using Ptr = std::shared_ptr<RdbProxy>;

	virtual void Dispose() override
	{
		Component::Dispose();
	}

	void AddConnection(uint16_t dbName, std::shared_ptr<pqxx::connection>&& connection)
	{
		pMdbProxys.emplace(dbName, std::move(connection));
	}

	std::shared_ptr<pqxx::connection> GetConnection(uint16_t dbName)
	{
		if (pMdbProxys.contains(dbName))
		{
			return pMdbProxys[dbName];
		}
		return nullptr;
	}

	LoggerPrint::Ptr GetLogger(){ return pLogger.expired() ? nullptr : pLogger.lock(); }

public:
	~RdbProxy() = default;

protected:
	std::unordered_map<uint16_t, std::shared_ptr<pqxx::connection>> pMdbProxys;

	LoggerPrint::WPtr pLogger;
};
