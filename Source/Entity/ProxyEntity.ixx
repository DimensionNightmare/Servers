export module ProxyEntity;

import ECSW;
import std.compat;
import ThirdParty.Libhv;

/// @brief this is client proxy entity
export class ProxyEntity : public Entity
{
protected:
	friend class UniversalMemoryPool;
	ProxyEntity(World::WPtr world):Entity(world)
	{
		eEntityType = EMEntityType::Proxy;
	}
public:
	using Ptr = std::shared_ptr<ProxyEntity>;
	virtual ~ProxyEntity()
	{
	}

protected: // dll proxy
	size_t iRecordServerId = 0;

	std::string sToken;

	int64_t iExpireTime = 0;

	size_t iCloseTimerId = 0;

	SocketChannel::Ptr pChannel;

};
