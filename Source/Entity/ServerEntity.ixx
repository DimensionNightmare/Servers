module;
#include "hv/Channel.h"
export module ServerEntity;

import Entity;
import DNServer;

using namespace hv;

export class ServerEntity : public Entity
{
public:
	ServerEntity();
	~ServerEntity();

public: // dll override
	virtual Entity* GetChild(){return this;}
	
protected: // dll proxy
    ServerType emServerType;
};

module:private;

ServerEntity::ServerEntity()
{
	emServerType = ServerType::None;
}

ServerEntity::~ServerEntity()
{
}
