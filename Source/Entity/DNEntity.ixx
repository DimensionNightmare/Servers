module;
#include <functional>
#include "hv/Channel.h"
export module DNEntity;

import Entity;

using namespace hv;
using namespace std;

export class DNEntity : public Entity
{
protected:
	DNEntity();

public:
	virtual ~DNEntity();

	auto& CloseEvent(){return pOnClose;}
public: // dll override

public:


protected: // dll proxy
	function<void(Entity*)> pOnClose;
	SocketChannelPtr pSock;
	uint64_t iCloseTimerId;
};

DNEntity::DNEntity()
{
	eEntityType = EntityType::None;

	pSock = nullptr;
	iCloseTimerId = 0;
	pOnClose = nullptr;
}

DNEntity::~DNEntity()
{
}
