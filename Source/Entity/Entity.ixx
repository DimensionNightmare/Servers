module;
#include "hv/Channel.h"

#include <functional>
export module Entity;

export enum class EntityType
{
	None,
	Server,
};

using namespace hv;
using namespace std;
export class Entity
{
public:
	Entity();
	virtual ~Entity();

	EntityType GetType(){return emType;}
	void TickCloseEvent(){if(pOnClose){pOnClose(this);}}
	void SetCloseEvent(function<void(Entity*)> func){ pOnClose = func;}
public: // dll override

public:


protected: // dll proxy
	function<void(Entity*)> pOnClose;
    unsigned int iId;
	SocketChannelPtr pSock;
	uint64_t iCloseTimerId;
	EntityType emType;
};



Entity::Entity()
{
	iId = 0;
	pSock = nullptr;
	iCloseTimerId = 0;
	pOnClose= nullptr;
	emType = EntityType::None;
}

Entity::~Entity()
{
}
