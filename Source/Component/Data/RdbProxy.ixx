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
	using CVPtr = const Ptr&;

	virtual void Dispose() override
	{
		Component::Dispose();
	}

	LoggerPrint::Ptr GetLogger(){ return pLogger.expired() ? nullptr : pLogger.lock(); }

public:
	~RdbProxy() = default;

protected:
	std::unordered_map<uint16_t, std::shared_ptr<pqxx::connection>> pMdbProxys;

	LoggerPrint::WPtr pLogger;
};
