module;
#include "hv/Channel.h"
export module ServerEntityHelper;

import ServerEntity;
import DNServer;
import EntityHelper;

using namespace hv;

export class ServerEntityHelper : public ServerEntity
{
private:
	ServerEntityHelper(){}
public:
	EntityHelper* GetChild(){ return nullptr;}

	void SetServerType(ServerType type){emServerType = type;}
	ServerType GetServerType(){ return emServerType;}
};

module:private;