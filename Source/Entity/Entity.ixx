module;
#include "hv/Channel.h"

export module Entity;

using namespace hv;

export class Entity
{
public:
	Entity();
	virtual ~Entity();

public: // dll override

protected: // dll proxy
    unsigned int iId;
	SocketChannelPtr pSock;
	uint64_t iCloseTimerId;
};

module:private;

Entity::Entity()
{
	iId = 0;
	pSock = nullptr;
	iCloseTimerId = 0;
}

Entity::~Entity()
{
}
