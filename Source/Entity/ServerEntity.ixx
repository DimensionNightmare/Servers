module;

#include <string>
#include <list>
#include <map>
#include <bitset>
export module ServerEntity;

import Entity;
import DNServer;

using namespace std;

export enum class ServerEntityFlag : int
{
	Locked = 0,
};

export class ServerEntity : public Entity
{
public:
	ServerEntity();
	virtual ~ServerEntity();

public: // dll override
	virtual Entity* GetChild(){return this;}

	ServerType GetType(){return emServerType;}

	ServerEntity* &LinkNode(){ return pLink;}

	bool HasFlag(ServerEntityFlag flag){ return oFlags.test(int(flag));}
	void SetFlag(ServerEntityFlag flag){ oFlags.set(int(flag));}
	void ClearFlag(ServerEntityFlag flag){ oFlags.reset(int(flag));}
	
protected: // dll proxy
    ServerType emServerType;
	string sServIp;
	unsigned short iServPort;
	unsigned int IConnNum;

	// regist node need
	ServerEntity* pLink;
	// be regist node need
	map<ServerType, list< ServerEntity*>> mMapLink;

	bitset<1> oFlags;
};



ServerEntity::ServerEntity():Entity()
{
	emServerType = ServerType::None;
	emType = EntityType::Server;
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
