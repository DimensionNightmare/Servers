module;
#include <string>
#include <list>
#include <map>
#include <bitset>
export module ServerEntity;

import NetEntity;
import DNServer;

using namespace std;

export enum class ServerEntityFlag : int
{
	Locked = 0,
};

export class ServerEntity : public NetEntity
{
public:
	ServerEntity();
	virtual ~ServerEntity();

public: // dll override

	ServerType GetType(){return emServerType;}

	ServerEntity* &LinkNode(){ return pLink;}

	bool HasFlag(ServerEntityFlag flag){ return oFlags.test(int(flag));}
	void SetFlag(ServerEntityFlag flag){ oFlags.set(int(flag));}
	void ClearFlag(ServerEntityFlag flag){ oFlags.reset(int(flag));}
	
protected: // dll proxy
    ServerType emServerType;
	string sServIp;
	uint16_t iServPort;
	uint32_t IConnNum;

	// regist node need
	ServerEntity* pLink;
	// be regist node need
	map<ServerType, list< ServerEntity*>> mMapLink;

	bitset<1> oFlags;
};

ServerEntity::ServerEntity()
{
	eEntityType = EntityType::Server;

	emServerType = ServerType::None;
	sServIp.clear();
	IConnNum = 0;
	iServPort = 0;
	pLink = nullptr;
	mMapLink.clear();
	oFlags.reset();
}

ServerEntity::~ServerEntity()
{
}
