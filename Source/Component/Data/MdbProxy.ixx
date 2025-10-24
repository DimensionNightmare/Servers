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
	using CVPtr = const Ptr&;
	
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
		World::CVPtr world = GetWorld();

		std::string* value = world->LaunchParam("connection");

		auto connection = P_InstanceHolder->MemPool->Allocate<sw::redis::Redis, const std::string&>(*value);
		
		connection->ping();

		pMdbProxys.emplace(0, std::move(connection));
	}

protected:

	std::unordered_map<uint16_t, std::shared_ptr<sw::redis::Redis>> pMdbProxys;
};
