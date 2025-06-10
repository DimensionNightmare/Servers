module;
export module MdbProxy;

import ThirdParty.RedisPP;
import ECSW;
import std.compat;
import Logger;

export class MdbProxy : public Component
{
protected:
	friend class System;
	MdbProxy(System::WPtr system):Component(system)
	{
		eComponentType = EMComponentType::MdbProxy;

		pLogger = GetOwner()->GetWorld()->GetSystemW<LoggerPrint>(EMSystemType::LoggerPrint);
	}

public:
	using Ptr = std::shared_ptr<MdbProxy>;
	using CVPtr = const Ptr&;
	
	~MdbProxy() = default;

	virtual void Dispose() override
	{
		Component::Dispose();
		
		pMdbProxys.clear();
	}

	LoggerPrint::Ptr GetLogger(){ return pLogger.expired() ? nullptr : pLogger.lock(); }

protected:

	std::unordered_map<uint16_t, std::shared_ptr<sw::redis::Redis>> pMdbProxys;

	LoggerPrint::WPtr pLogger;

};
