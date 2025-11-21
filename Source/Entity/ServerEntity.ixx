export module ServerEntity;

import BitFlag;
import ECSW;
import std.compat;
import Server;
import ThirdParty.Libhv;
import Logger;

export enum class EMServerEntityFlag : uint16_t
{
	None = 0,
	Locked,
	Max,
};

/// @brief this is server proxy entity
export class ServerEntity : public Entity, public BitFlag<EMServerEntityFlag>
{
protected:
	friend class UniversalMemoryPool;
	ServerEntity(World::CVPtr world):Entity(world)
	{
		eEntityType = EMEntityType::Server;
	}
public:
	using Ptr = std::shared_ptr<ServerEntity>;
	using CVPtr = const Ptr&;

	virtual ~ServerEntity()
	{
		
	}

	virtual void Dispose() override
	{
		pLink = nullptr;
		mMapLink.clear();
		
		Entity::Dispose();
	}
	
	/// @brief this server father node
	ServerEntity::CVPtr LinkNode() { return pLink; }

	/// @brief this server childs get
	std::list<ServerEntity::Ptr>& GetMapLinkNode(EMServerType type) { return mMapLink[type]; }
	
	/// 
	EMServerType GetServerType() { return emServerType; }

	SocketChannel::CVPtr GetChannel() { return pChannel; }

protected: // dll proxy
	EMServerType emServerType = EMServerType::None;

	std::string sServIp;
	uint16_t iServPort = 0;
	uint32_t IConnNum = 0;

	// regist node need
	ServerEntity::Ptr pLink;
	// be regist node need
	std::unordered_map<EMServerType, std::list<ServerEntity::Ptr>> mMapLink;

	size_t iCloseTimerId = 0;

	SocketChannel::Ptr pChannel;
};
