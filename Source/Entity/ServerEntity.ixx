module;

#include <string>
export module ServerEntity;

import Entity;
import DNServer;

export class ServerEntity : public Entity
{
public:
	ServerEntity();
	~ServerEntity();

public: // dll override
	virtual Entity* GetChild(){return this;}
	
protected: // dll proxy
    ServerType emServerType;
	std::string sIpAddr;
};

module:private;

ServerEntity::ServerEntity()
{
	emServerType = ServerType::None;
	sIpAddr = "";
}

ServerEntity::~ServerEntity()
{
}
