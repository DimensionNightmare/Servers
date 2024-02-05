module;

#include <string>
export module ServerEntityHelper;

import DNServer;
import ServerEntity;
import EntityHelper;

using namespace std;

export class ServerEntityHelper : public ServerEntity
{
private:
	ServerEntityHelper(){}
public:
	EntityHelper* GetChild(){ return nullptr;}

	void SetServerType(ServerType type){emServerType = type;}
	ServerType GetServerType(){ return emServerType;}

	void SetServerIp(const std::string& ip){sServIp = ip;}
	std::string& GetServerIp(){ return sServIp;}

	void SetServerPort(unsigned short port){iServPort = port;}
	unsigned short GetServerPort(){ return iServPort;}

	void SetConnNum(unsigned int num){IConnNum = num;}
	unsigned int GetConnNum(){ return IConnNum;}

	void SetLinkNode(ServerEntity* node){pLink = node;}
	ServerEntity* GetLinkNode(){ return pLink;}
	
	void SetMapLinkNode(ServerType type, ServerEntity* node)
	{
		if(type <= ServerType::None || type >= ServerType::Max)
		{
			return;
		}

		mMapLink[type].emplace_back(node);
	}

	list<ServerEntity*>& GetMapLinkNode(ServerType type)
	{
		return mMapLink[type];
	}
	
};

module:private;