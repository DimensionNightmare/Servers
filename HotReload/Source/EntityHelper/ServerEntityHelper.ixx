module;

#include <string>
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

	void SetServerIp(std::string& type){sIpAddr = type;}
	std::string& GetServerIp(){ return sIpAddr;}

	void SetConnNum(unsigned int num){IConnNum = num;}
	unsigned int GetConnNum(){ return IConnNum;}

	void SetLinkNode(ServerEntity* node){pLink = node;}
	ServerEntity* GetLinkNode(){ return pLink;}
};

module:private;