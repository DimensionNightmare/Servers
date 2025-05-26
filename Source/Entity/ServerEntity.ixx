module;
export module ServerEntity;

import ECSW;

import DNServer;
import std.compat;
import ThirdParty.Libhv;
import BitFlag;

export enum class EMServerEntityFlag : uint16_t
{
	Locked = 0,
	Max,
};

/// @brief this is server proxy entity
export class ServerEntity : public Entity, public BitFlag<EMServerEntityFlag>
{
protected:
	friend class ServerEntityManagerHelper;
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

	void SetServerType(EMServerType type) { emServerType = type; }

	/// @brief this server father node
	const ServerEntity::Ptr& LinkNode() { return pLink; }
	void SetLinkNode(const ServerEntity::Ptr& node) { pLink = node; }

	std::string ServerIp() { return sServIp; }
	void SetServerIp(const std::string& ip) { sServIp = ip; }

	uint16_t ServerPort() { return iServPort; }
	void SetServerPort(uint16_t port) { iServPort = port; }

	/// @brief this server connected clients num
	uint32_t ConnNum() { return IConnNum; }
	void SetConnNum(int div) { IConnNum += div; }

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
	uint64_t TimerId() { return iCloseTimerId; }
	void SetTimerId(uint64_t timerId) { iCloseTimerId = timerId; }

	/// @brief net socket set
	const DNSocketChannel::Ptr& GetChannel() { return pChannel; }

	/// @brief net socket get
	void SetChannel(const DNSocketChannel::Ptr& channel) { pChannel = channel; }

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
