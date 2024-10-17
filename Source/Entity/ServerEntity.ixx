module;
#include "StdMacro.h"
export module ServerEntity;

import NetEntity;
export import DNServer;

export enum class ServerEntityFlag : uint16_t
{
	Locked = 0,
	Max,
};

constexpr uint16_t ServerEntityFlagSize() { return static_cast<uint16_t>(ServerEntityFlag::Max); }

/// @brief this is server proxy entity
export class ServerEntity : public NetEntity
{
public:

	ServerEntity() :NetEntity(0)
	{
		eEntityType = EntityType::Server;
	}

	ServerEntity(uint32_t id, ServerType serverType) :NetEntity(id)
	{
		eEntityType = EntityType::Server;
		emServerType = serverType;
	}

	virtual ~ServerEntity()
	{
		pLink = nullptr;
		mMapLink.clear();
	}
	
public: // dll override
	/// 
	ServerType GetServerType() { return emServerType; }

	/// @brief this server father node
	ServerEntity*& LinkNode() { return pLink; }

	bool HasFlag(ServerEntityFlag flag) { return oFlags.test(uint16_t(flag)); }
	void SetFlag(ServerEntityFlag flag) { oFlags.set(uint16_t(flag)); }
	void ClearFlag(ServerEntityFlag flag) { oFlags.reset(uint16_t(flag)); }

	string& ServerIp() { return sServIp; }

	uint16_t& ServerPort() { return iServPort; }

	/// @brief this server connected clients num
	uint32_t& ConnNum() { return IConnNum; }

	/// @brief this server child add
	void SetMapLinkNode(ServerType type, ServerEntity* node)
	{
		if (type <= ServerType::None || type >= ServerType::Max)
		{
			return;
		}

		mMapLink[type].emplace_back(node);
	}

	/// @brief this server childs get
	list<ServerEntity*>& GetMapLinkNode(ServerType type) { return mMapLink[type]; }

protected: // dll proxy
	ServerType emServerType = ServerType::None;

	string sServIp;
	uint16_t iServPort = 0;
	uint32_t IConnNum = 0;

	// regist node need
	ServerEntity* pLink = nullptr;
	// be regist node need
	unordered_map<ServerType, list<ServerEntity*>> mMapLink;

	bitset<ServerEntityFlagSize()> oFlags;

};
