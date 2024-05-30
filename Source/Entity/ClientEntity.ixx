module;
#include <cstdint>
export module ClientEntity;

import Entity;

export class ClientEntity : public Entity
{
public:
	ClientEntity();
	ClientEntity(uint32_t id);
	virtual ~ClientEntity();

public: // dll override
	void Load();

	void Save();

	uint32_t& ServerIndex() { return iServerIndex; }

protected: // dll proxy
	uint32_t iServerIndex;
};

ClientEntity::ClientEntity()
{
	eEntityType = EntityType::Client;
	iServerIndex = 0;
}

ClientEntity::ClientEntity(uint32_t id) :Entity(id)
{
	eEntityType = EntityType::Client;
	iServerIndex = 0;
}

ClientEntity::~ClientEntity()
{

}

void ClientEntity::Load()
{

}

void ClientEntity::Save()
{
}
