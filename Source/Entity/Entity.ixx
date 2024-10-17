module;
#include "StdMacro.h"
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

// normal data normal get/set
// if class function has logic. please imp to helper.

export class Entity
{

public:
	/// @brief must give id 
	Entity(uint32_t id)
	{
		iId = id;
	}

	virtual ~Entity()
	{
	}
	
public: // dll override

	uint32_t ID() { return iId; }

	/// @brief entity type total enum
	EntityType GetEntityType() { return eEntityType; }

protected: // dll proxy

protected:

	EntityType eEntityType = EntityType::None;

	uint32_t iId = 0;

};
