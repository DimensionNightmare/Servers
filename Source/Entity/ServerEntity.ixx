module;
export module ServerEntity;

import NetEntity;
import DNServer;
import std.compat;

export enum class EMServerEntityFlag : uint16_t
{
	Locked = 0,
	Max,
};

using ServerEntityBitFlag = std::bitset<static_cast<uint16_t>(EMServerEntityFlag::Max)>;

/// @brief this is server proxy entity
export class ServerEntity : public NetEntity
{
protected:
	ServerEntity(World::WPtr world):NetEntity(world)
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
	
public: // dll override
	/// 
	EMServerType GetServerType() { return emServerType; }

	/// @brief this server father node
	ServerEntity::Ptr LinkNode() { return pLink; }

	bool HasFlag(EMServerEntityFlag flag) { return oFlags.test(uint16_t(flag)); }
	void SetFlag(EMServerEntityFlag flag) { oFlags.set(uint16_t(flag)); }
	void ClearFlag(EMServerEntityFlag flag) { oFlags.reset(uint16_t(flag)); }

	std::string& ServerIp() { return sServIp; }

	uint16_t& ServerPort() { return iServPort; }

	/// @brief this server connected clients num
	uint32_t& ConnNum() { return IConnNum; }

	/// @brief this server child add
	void SetMapLinkNode(EMServerType type, ServerEntity* node)
	{
		if (type <= EMServerType::None || type >= EMServerType::Max)
		{
			return;
		}

		mMapLink[type].emplace_back(node);
	}

	/// @brief this server childs get
	std::list<ServerEntity::Ptr>& GetMapLinkNode(EMServerType type) { return mMapLink[type]; }

protected: // dll proxy
	EMServerType emServerType = EMServerType::None;

	std::string sServIp;
	uint16_t iServPort = 0;
	uint32_t IConnNum = 0;

	// regist node need
	ServerEntity::Ptr pLink = nullptr;
	// be regist node need
	std::unordered_map<EMServerType, std::list<ServerEntity::Ptr>> mMapLink;

	ServerEntityBitFlag oFlags;

};
