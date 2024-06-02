module;
#include <cstdint>
#include "hv/Channel.h"
export module NetEntity;

export import Entity;

using namespace hv;
using namespace std;

export class NetEntity : public Entity
{
protected:
	NetEntity(uint32_t id);

public:
	virtual ~NetEntity();

public: // dll override
	const SocketChannelPtr& GetSock() { return pSock; }
	void SetSock(const SocketChannelPtr& channel) { pSock = channel; }

	uint64_t& TimerId() { return iCloseTimerId; }

protected: // dll proxy
	SocketChannelPtr pSock;

	uint64_t iCloseTimerId = 0;
};

NetEntity::NetEntity(uint32_t id) : Entity(id)
{}

NetEntity::~NetEntity()
{
	pSock = nullptr;
}
