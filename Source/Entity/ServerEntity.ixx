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

constexpr size_t ServerEntityFlagSize()
{
	return static_cast<size_t>(ServerEntityFlag::Max);
}

export class ServerEntity : public NetEntity
{
public:
	ServerEntity();
	ServerEntity(uint32_t id, ServerType serverType);
	virtual ~ServerEntity();

public: // dll override

	ServerType GetServerType() { return emServerType; }

	ServerEntity*& LinkNode() { return pLink; }

	bool HasFlag(ServerEntityFlag flag) { return oFlags.test(int(flag)); }
	void SetFlag(ServerEntityFlag flag) { oFlags.set(int(flag)); }
	void ClearFlag(ServerEntityFlag flag) { oFlags.reset(int(flag)); }

	string& ServerIp() { return sServIp; }

	uint16_t& ServerPort() { return iServPort; }

	uint32_t& ConnNum() { return IConnNum; }

	void SetMapLinkNode(ServerType type, ServerEntity* node);

	list<ServerEntity*>& GetMapLinkNode(ServerType type) { return mMapLink[type]; }

protected: // dll proxy
	ServerType emServerType;
	string sServIp;
	uint16_t iServPort;
	uint32_t IConnNum;

	// regist node need
	ServerEntity* pLink;
	// be regist node need
	unordered_map<ServerType, list<ServerEntity*>> mMapLink;

	bitset<ServerEntityFlagSize()> oFlags;
};

export using ServerEntityPtr = ServerEntity*;

ServerEntity::ServerEntity()
{
	eEntityType = EntityType::Server;
	emServerType = ServerType::None;
	IConnNum = 0;
	iServPort = 0;
	pLink = nullptr;
}

ServerEntity::ServerEntity(uint32_t id, ServerType serverType) :NetEntity(id)
{
	emServerType = serverType;
	eEntityType = EntityType::Server;
	IConnNum = 0;
	iServPort = 0;
	pLink = nullptr;
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
