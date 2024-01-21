module;

#include <string>
export module ServerEntity;

import Entity;
import DNServer;

using namespace std;

export class ServerEntity : public Entity
{
public:
	ServerEntity();
	~ServerEntity();

public: // dll override
	virtual Entity* GetChild(){return this;}
	
protected: // dll proxy
    ServerType emServerType;
	string sServIp;
	unsigned short iServPort;
	unsigned int IConnNum;

	ServerEntity* pLink;
};

module:private;

ServerEntity::ServerEntity()
{
	emServerType = ServerType::None;
	sServIp = "";
	IConnNum = 0;
	iServPort = 0;
	pLink = nullptr;
}

ServerEntity::~ServerEntity()
{
}
