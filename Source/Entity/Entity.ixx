module;
#include "hv/Channel.h"
export module Entity;

using namespace hv;

export class Entity
{
    
public:
    unsigned int iId;
	SocketChannelPtr pSock;
};

module:private;