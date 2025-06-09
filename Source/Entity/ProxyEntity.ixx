module;
export module ProxyEntity;

import ECSW;
import std.compat;
import ThirdParty.Libhv;

/// @brief this is client proxy entity
export class ProxyEntity : public Entity
{
protected:
	friend class ProxyEntityManager;
	ProxyEntity(World::WPtr world):Entity(world)
	{
		eEntityType = EMEntityType::Proxy;
	}
public:
	using Ptr = std::shared_ptr<ProxyEntity>;
	using CVPtr = const Ptr&;
	virtual ~ProxyEntity()
	{
	}

protected: // dll proxy
	uint64_t iRecordServerId = 0;

	std::string sToken;

	int64_t iExpireTime = 0;

	uint64_t iCloseTimerId = 0;

	DNSocketChannel::Ptr pChannel;

};
