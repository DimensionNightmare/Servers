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
	auto GetServerIp(){ return sIpAddr;}

	void SetConnNum(unsigned int num){IConnNum = num;}
	auto GetConnNum(){ return IConnNum;}
};

module:private;