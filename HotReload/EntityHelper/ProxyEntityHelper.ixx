module;
#include <string>
#include "hv/Channel.h"
export module ProxyEntityHelper;

export import ProxyEntity;

using namespace std;
using namespace hv;

export class ProxyEntityHelper : public ProxyEntity
{
private:
	ProxyEntityHelper(){}
public:
	string& Token(){ return sToken; }

	uint32_t& ExpireTime(){ return iExpireTime; }

	uint32_t& ID(){ return iId;}

	void SetSock(const SocketChannelPtr& channel){ pSock = channel;}

	const SocketChannelPtr& GetSock(){return pSock;}

	uint64_t& TimerId(){return iCloseTimerId;}

	uint32_t& ServerIndex(){ return iServerIndex; }
};

