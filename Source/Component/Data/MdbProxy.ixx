export module MdbProxy;

import ThirdParty.RedisPP;
import ECSW;
import std.compat;
import Logger;

export class MdbProxy : public Component
{
protected:
	friend class UniversalMemoryPool;
	MdbProxy(System::WPtr system):Component(system)
	{
		eComponentType = EMComponentType::MdbProxy;
	}

public:
	using Ptr = std::shared_ptr<MdbProxy>;
	
	virtual ~MdbProxy()
	{
	}

	virtual void Dispose() override
	{
		Component::Dispose();
		
		pMdbProxys.clear();
	}

	virtual bool Awake() override
	{
		GetOwner()->AddEvent(EMEventType::ServerStart, GetSelfW<MdbProxy>(), &MdbProxy::InitDatabase);
		return true;
	}

	void InitDatabase()
	{
		World::Ptr world = GetWorld();

		std::string* value = world->LaunchParam("connection");

		auto connection = P_InstanceHolder->GetMemPool().Allocate<sw::redis::Redis>(*value);
		
		connection->ping();

		pMdbProxys.emplace(0, std::move(connection));
	}

protected:

	std::unordered_map<uint16_t, std::shared_ptr<sw::redis::Redis>> pMdbProxys;
};
