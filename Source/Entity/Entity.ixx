module;
#include <functional>
#include <cstdint>
#include "hv/Channel.h"
export module Entity;

export enum class EntityType : uint8_t
{
	None,
	// NetEntity, virtual
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

protected: // dll proxy

public:
	EntityType eEntityType;
	uint32_t iId;
};

Entity::Entity()
{
	eEntityType = EntityType::None;
	iId = 0;
}

Entity::~Entity()
{
}
