module;
#include <string>
#include <cstdint>
export module ProxyEntity;

import NetEntity;

using namespace std;

export class ProxyEntity : public NetEntity
{
public:
	ProxyEntity();
	ProxyEntity(uint32_t id);

	virtual ~ProxyEntity();

public: // dll override
	string& Token() { return sToken; }

	uint32_t& ExpireTime() { return iExpireTime; }

	uint32_t& ServerIndex() { return iServerIndex; }

protected: // dll proxy
	uint32_t iServerIndex;

	string sToken;

	uint32_t iExpireTime;
};



ProxyEntity::ProxyEntity()
{
	eEntityType = EntityType::Proxy;
	iServerIndex = 0;
	iExpireTime = 0;
}

ProxyEntity::ProxyEntity(uint32_t id) :NetEntity(id)
{
	eEntityType = EntityType::Proxy;
	iServerIndex = 0;
	iExpireTime = 0;
}

ProxyEntity::~ProxyEntity()
{
}
