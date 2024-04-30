module;
#include <cstdint>
export module ClientEntity;

import Entity;

export class ClientEntity : public Entity
{
public:
	ClientEntity();
	virtual ~ClientEntity();

public: // dll override

protected: // dll proxy
	uint32_t iServerIndex;
};

ClientEntity::ClientEntity()
{
	eEntityType = EntityType::Client;
	iServerIndex = 0;
}

ClientEntity::~ClientEntity()
{
}
