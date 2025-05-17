module;
export module ServerEntity;

import ECSW;

import DNServer;
import std.compat;
import DNSocketProxy;

export enum class EMServerEntityFlag : uint16_t
{
	Locked = 0,
	Max,
};

using ServerEntityBitFlag = std::bitset<static_cast<uint16_t>(EMServerEntityFlag::Max)>;

/// @brief this is server proxy entity
export class ServerEntity : public Entity
{
protected:
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
	void SetMapLinkNode(EMServerType type, const ServerEntity::Ptr& node)
	{
		if (type <= EMServerType::None || type >= EMServerType::Max)
		{
			return;
		}

		mMapLink[type].emplace_back(node);
	}

	/// @brief this server childs get
	std::list<ServerEntity::Ptr>& GetMapLinkNode(EMServerType type) { return mMapLink[type]; }

	/// @brief the this close timedown destroy timerid.
	/// @brief authenticate,shutdown and reconnect waiting.
	uint64_t& TimerId() { return iCloseTimerId; }

	/// @brief net socket set
	const DNSocketProxy::Ptr& GetSock() { return pSock; }

	/// @brief net socket get
	void SetSock(const DNSocketProxy::Ptr& sock) { pSock = sock; }

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

	uint64_t iCloseTimerId = 0;

	DNSocketProxy::Ptr pSock;
};
