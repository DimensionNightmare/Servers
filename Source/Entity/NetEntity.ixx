module;
#include <functional>
#include "hv/Channel.h"
export module NetEntity;

import Entity;

using namespace hv;
using namespace std;

export class NetEntity : public Entity
{
protected:
	NetEntity();

public:
	virtual ~NetEntity();

public: // dll override
	auto& CloseEvent(){return pOnClose;}

protected: // dll proxy
	function<void(Entity*)> pOnClose;
	SocketChannelPtr pSock;
	uint64_t iCloseTimerId;
};

NetEntity::NetEntity()
{
	eEntityType = EntityType::None;

	pSock = nullptr;
	iCloseTimerId = 0;
	pOnClose = nullptr;
}

NetEntity::~NetEntity()
{
}
