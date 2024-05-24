module;
#include <string>
#include <cstdint>
#include <list>
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

	uint16_t& ServerPort(){ return iServPort;}

	uint32_t& GetConnNum(){ return IConnNum;}
	
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
	
	uint32_t& ID(){ return iId;}

	void SetSock(SocketChannelPtr channel){ pSock = channel;}
	SocketChannelPtr GetSock(){return pSock;}

	uint64_t& TimerId(){return iCloseTimerId;}
};

