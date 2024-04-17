module;

#include <bitset>
export module ProxyEntity;

import Entity;

using namespace std;

export class ProxyEntity : public Entity
{
public:
	ProxyEntity();
	virtual ~ProxyEntity();

public: // dll override
	virtual Entity* GetChild(){return this;}

protected: // dll proxy
	unsigned int iServerIndex;

	bitset<1> oFlags;

	string sToken;
	unsigned int iExpireTime;
};



ProxyEntity::ProxyEntity():Entity()
{
	iServerIndex = 0;
	oFlags.reset();
	sToken.clear();
	iExpireTime = 0;

	emType = EntityType::Proxy;
}

ProxyEntity::~ProxyEntity()
{
}
