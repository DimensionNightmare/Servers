module;
export module ServerEntity;

import BitFlag;
import ECSW;
import std.compat;
import DNServer;
import ThirdParty.Libhv;

export enum class EMServerEntityFlag : uint16_t
{
	Locked = 0,
	Max,
};

/// @brief this is server proxy entity
export class ServerEntity : public Entity, public BitFlag<EMServerEntityFlag>
{
protected:
	friend class ServerEntityManager;
	ServerEntity(World::WPtr world):Entity(world)
	{
		eEntityType = EMEntityType::Server;
	}
public:
	using Ptr = std::shared_ptr<ServerEntity>;
	virtual ~ServerEntity()
	{
		pLink = nullptr;
		mMapLink.clear();
	}
	
	/// @brief this server father node
	const ServerEntity::Ptr& LinkNode() { return pLink; }

	/// @brief this server childs get
	std::list<ServerEntity::Ptr>& GetMapLinkNode(EMServerType type) { return mMapLink[type]; }
	
	/// 
	EMServerType GetServerType() { return emServerType; }

protected: // dll proxy
	EMServerType emServerType = EMServerType::None;

	std::string sServIp;
	uint16_t iServPort = 0;
	uint32_t IConnNum = 0;

	// regist node need
	ServerEntity::Ptr pLink;
	// be regist node need
	std::unordered_map<EMServerType, std::list<ServerEntity::Ptr>> mMapLink;

	uint64_t iCloseTimerId = 0;

	DNSocketChannel::Ptr pChannel;
};
