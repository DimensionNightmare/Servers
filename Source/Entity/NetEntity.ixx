module;
#include <functional>
#include <cstdint>
#include "hv/Channel.h"
export module NetEntity;

export import Entity;

using namespace hv;
using namespace std;

export class NetEntity : public Entity
{
protected:
	NetEntity();

public:
	virtual ~NetEntity();

public: // dll override

protected: // dll proxy
	SocketChannelPtr pSock;
	uint64_t iCloseTimerId;
};

NetEntity::NetEntity()
{
	eEntityType = EntityType::None;

	pSock = nullptr;
	iCloseTimerId = 0;
}

NetEntity::~NetEntity()
{
}
