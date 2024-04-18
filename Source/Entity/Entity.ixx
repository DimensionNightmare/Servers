module;
#include <functional>
#include "hv/Channel.h"
export module Entity;

export enum class EntityType
{
	None,
	// DNServer, virtual
	Server,
	Proxy,
	Client,
	Room,
};

using namespace hv;
using namespace std;

export class Entity
{
public:
	Entity();
	virtual ~Entity();
public: // dll override

public:


protected: // dll proxy
	EntityType eEntityType;
    unsigned int iId;
};

Entity::Entity()
{
	eEntityType = EntityType::None;
	iId = 0;
}

Entity::~Entity()
{
}
