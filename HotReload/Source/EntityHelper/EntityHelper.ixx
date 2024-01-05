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
	auto GetID(){ return iId;}

	void SetSock(const SocketChannelPtr& channel){ pSock = channel;}
	const SocketChannelPtr& GetSock(){return pSock;}
};

module:private;