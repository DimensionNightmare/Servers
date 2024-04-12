module;

#include <string>
export module ServerEntityHelper;

import DNServer;
export import ServerEntity;
import EntityHelper;

using namespace std;

export class ServerEntityHelper : public ServerEntity
{
private:
	ServerEntityHelper(){}
public:
	EntityHelper* GetChild(){ return nullptr;}

	ServerType& ServerEntityType(){ return emServerType;}

	std::string& ServerIp(){ return sServIp;}

	unsigned short& ServerPort(){ return iServPort;}

	unsigned int& GetConnNum(){ return IConnNum;}
	
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

