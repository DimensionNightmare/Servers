module;
#include "StdMacro.h"
export module ServerEntity;

import NetEntity;
import DNServer;

export enum class EMServerEntityFlag : uint16_t
{
	Locked = 0,
	Max,
};

constexpr uint16_t ServerEntityFlagSize() { return static_cast<uint16_t>(EMServerEntityFlag::Max); }

/// @brief this is server proxy entity
export class ServerEntity : public NetEntity
{
	
public:

	ServerEntity() :NetEntity(0)
	{
		eEntityType = EMEntityType::Server;
	}

	ServerEntity(uint32_t id, EMServerType serverType) :NetEntity(id)
	{
		eEntityType = EMEntityType::Server;
		emServerType = serverType;
	}

	virtual ~ServerEntity()
	{
		pLink = nullptr;
		mMapLink.clear();
	}
	
public: // dll override
	/// 
	EMServerType GetServerType() { return emServerType; }

	/// @brief this server father node
	ServerEntity*& LinkNode() { return pLink; }

	bool HasFlag(EMServerEntityFlag flag) { return oFlags.test(uint16_t(flag)); }
	void SetFlag(EMServerEntityFlag flag) { oFlags.set(uint16_t(flag)); }
	void ClearFlag(EMServerEntityFlag flag) { oFlags.reset(uint16_t(flag)); }

	string& ServerIp() { return sServIp; }

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
	list<ServerEntity*>& GetMapLinkNode(EMServerType type) { return mMapLink[type]; }

protected: // dll proxy
	EMServerType emServerType = EMServerType::None;

	string sServIp;
	uint16_t iServPort = 0;
	uint32_t IConnNum = 0;

	// regist node need
	ServerEntity* pLink = nullptr;
	// be regist node need
	unordered_map<EMServerType, list<ServerEntity*>> mMapLink;

	bitset<ServerEntityFlagSize()> oFlags;

};
