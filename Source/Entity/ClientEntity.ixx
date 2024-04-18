module;
export module ClientEntity;

import Entity;

export class ClientEntity : public Entity
{
public:
	ClientEntity();
	virtual ~ClientEntity();

public: // dll override

protected: // dll proxy
};

ClientEntity::ClientEntity()
{
	eEntityType = EntityType::Client;
}

ClientEntity::~ClientEntity()
{
}
