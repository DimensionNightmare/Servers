module;

#include <string>
#include <list>
#include <map>
export module ServerEntity;

import Entity;
import DNServer;

using namespace std;

export class ServerEntity : public Entity
{
public:
	ServerEntity();
	virtual ~ServerEntity();

public: // dll override
	virtual Entity* GetChild(){return this;}
	
protected: // dll proxy
    ServerType emServerType;
	string sServIp;
	unsigned short iServPort;
	unsigned int IConnNum;

	// regist node need
	ServerEntity* pLink;
	// be regist node need
	map<ServerType, list< ServerEntity*>> mMapLink;
};

module:private;

ServerEntity::ServerEntity()
{
	emServerType = ServerType::None;
	sServIp.clear();
	IConnNum = 0;
	iServPort = 0;
	pLink = nullptr;
	mMapLink.clear();
}

ServerEntity::~ServerEntity()
{
}
