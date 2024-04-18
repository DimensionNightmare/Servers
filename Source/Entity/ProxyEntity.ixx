module;
#include <string>
export module ProxyEntity;

import NetEntity;

using namespace std;

export class ProxyEntity : public NetEntity
{
public:
	ProxyEntity();
	virtual ~ProxyEntity();

public: // dll override

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
