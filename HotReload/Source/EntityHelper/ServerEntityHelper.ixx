module;
export module ServerEntityHelper;

import DNServer;
import ServerEntity;
import EntityHelper;

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