module;
#include "hv/Channel.h"

export module EntityHelper;

export import Entity;

using namespace hv;

export class EntityHelper : public Entity
{
private:
	EntityHelper(){};

public:
	unsigned int& ID(){ return iId;}

	void SetSock(const SocketChannelPtr& channel){ pSock = channel;}
	const SocketChannelPtr& GetSock(){return pSock;}

	uint64_t& TimerId(){return iCloseTimerId;}
};

module:private;