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
};

module:private;

Entity::Entity()
{
	iId = 0;
	pSock = nullptr;
}

Entity::~Entity()
{
}
