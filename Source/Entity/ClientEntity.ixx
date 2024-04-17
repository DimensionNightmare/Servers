module;

#include <bitset>
export module ClientEntity;

import Entity;

using namespace std;

export class ClientEntity : public Entity
{
public:
	ClientEntity();
	virtual ~ClientEntity();

public: // dll override
	virtual Entity* GetChild(){return this;}

protected: // dll proxy
	
};



ClientEntity::ClientEntity():Entity()
{
	emType = EntityType::Client;
}

ClientEntity::~ClientEntity()
{
}
