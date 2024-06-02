module;
#include <cstdint>
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

using namespace std;

// normal data normal get/set
// if class function has logic. please imp to helper.

export class Entity
{
public:
	Entity(uint32_t id);
	virtual ~Entity();
public: // dll override
	uint32_t ID() { return iId; }
	EntityType GetEntityType() { return eEntityType; }
protected: // dll proxy

protected:
	EntityType eEntityType = EntityType::None;
	uint32_t iId = 0;
};

Entity::Entity(uint32_t id)
{
	iId = id;
}

Entity::~Entity()
{}
