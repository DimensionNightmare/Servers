module;
#include "hv/Channel.h"
export module DNEntityHelper;

export import DNEntity;

using namespace hv;

export class DNEntityHelper : public DNEntity
{
private:
	DNEntityHelper(){};

public:
	unsigned int& ID(){ return iId;}

	void SetSock(const SocketChannelPtr& channel){ pSock = channel;}
	const SocketChannelPtr& GetSock(){return pSock;}

	uint64_t& TimerId(){return iCloseTimerId;}
};

