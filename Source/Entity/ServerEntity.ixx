module;
#include <string>
#include <list>
#include <unordered_map>
#include <bitset>
#include <cstdint>
export module ServerEntity;

import NetEntity;
export import DNServer;

using namespace std;

export enum class ServerEntityFlag : uint16_t
{
	Locked = 0,
	Max,
};

constexpr uint16_t ServerEntityFlagSize() { return static_cast<uint16_t>(ServerEntityFlag::Max); }

export class ServerEntity : public NetEntity
{
public:
	ServerEntity();
	ServerEntity(uint32_t id, ServerType serverType);
	virtual ~ServerEntity();

public: // dll override

	ServerType GetServerType() { return emServerType; }

	ServerEntity*& LinkNode() { return pLink; }

	bool HasFlag(ServerEntityFlag flag) { return oFlags.test(uint16_t(flag)); }
	void SetFlag(ServerEntityFlag flag) { oFlags.set(uint16_t(flag)); }
	void ClearFlag(ServerEntityFlag flag) { oFlags.reset(uint16_t(flag)); }

	string& ServerIp() { return sServIp; }

	uint16_t& ServerPort() { return iServPort; }

	uint32_t& ConnNum() { return IConnNum; }

	void SetMapLinkNode(ServerType type, ServerEntity* node);

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

ServerEntity::ServerEntity() :NetEntity(0)
{
	eEntityType = EntityType::Server;
}

ServerEntity::ServerEntity(uint32_t id, ServerType serverType) :NetEntity(id)
{
	eEntityType = EntityType::Server;
	emServerType = serverType;
}

ServerEntity::~ServerEntity()
{
	pLink = nullptr;
	mMapLink.clear();
}

void ServerEntity::SetMapLinkNode(ServerType type, ServerEntity* node)
{
	if (type <= ServerType::None || type >= ServerType::Max)
	{
		return;
	}

	mMapLink[type].emplace_back(node);
}
