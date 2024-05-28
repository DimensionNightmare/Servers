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
	virtual ~ServerEntity();

public: // dll override

	ServerType GetType() { return emServerType; }

	ServerEntity*& LinkNode() { return pLink; }

	bool HasFlag(ServerEntityFlag flag) { return oFlags.test(int(flag)); }
	void SetFlag(ServerEntityFlag flag) { oFlags.set(int(flag)); }
	void ClearFlag(ServerEntityFlag flag) { oFlags.reset(int(flag)); }

protected: // dll proxy
	ServerType emServerType;
	string sServIp;
	uint16_t iServPort;
	uint32_t IConnNum;

	// regist node need
	ServerEntity* pLink;
	// be regist node need
	unordered_map<ServerType, list< ServerEntity*>> mMapLink;

	bitset<ServerEntityFlagSize()> oFlags;
};

ServerEntity::ServerEntity()
{
	eEntityType = EntityType::Server;

	emServerType = ServerType::None;
	IConnNum = 0;
	iServPort = 0;
	pLink = nullptr;
}

ServerEntity::~ServerEntity()
{
	pLink = nullptr;
	mMapLink.clear();
}
