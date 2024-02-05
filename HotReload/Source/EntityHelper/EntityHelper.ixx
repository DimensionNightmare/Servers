module;
#include "hv/Channel.h"

export module EntityHelper;

import Entity;

using namespace hv;

export class EntityHelper : public Entity
{
private:
	EntityHelper(){};

public:
	void SetID(unsigned int id){iId = id;}
	unsigned int GetID(){ return iId;}

	void SetSock(const SocketChannelPtr& channel){ pSock = channel;}
	const SocketChannelPtr& GetSock(){return pSock;}

	void SetTimerId(uint64_t timerId){iCloseTimerId = timerId;}
	uint64_t GetTimerId(){return iCloseTimerId;}
};

module:private;