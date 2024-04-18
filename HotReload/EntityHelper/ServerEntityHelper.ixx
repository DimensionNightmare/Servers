module;
#include <string>
#include "hv/Channel.h"
export module ServerEntityHelper;

export import ServerEntity;

using namespace std;
using namespace hv;

export class ServerEntityHelper : public ServerEntity
{
private:
	ServerEntityHelper(){}
public:
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
	
	unsigned int& ID(){ return iId;}

	void SetSock(const SocketChannelPtr& channel){ pSock = channel;}
	const SocketChannelPtr& GetSock(){return pSock;}

	uint64_t& TimerId(){return iCloseTimerId;}
};

