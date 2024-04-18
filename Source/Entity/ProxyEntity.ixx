module;
#include <string>
export module ProxyEntity;

import DNEntity;

using namespace std;

export class ProxyEntity : public DNEntity
{
public:
	ProxyEntity();
	virtual ~ProxyEntity();

public: // dll override
	virtual DNEntity* GetChild(){return this;}

protected: // dll proxy
	unsigned int iServerIndex;

	string sToken;
	unsigned int iExpireTime;
};



ProxyEntity::ProxyEntity()
{
	eEntityType = EntityType::Proxy;

	iServerIndex = 0;
	sToken.clear();
	iExpireTime = 0;
}

ProxyEntity::~ProxyEntity()
{
}
