module;
export module ProxyEntity;

import NetEntity;


/// @brief this is client proxy entity
export class ProxyEntity : public NetEntity
{
protected:
	ProxyEntity(World::Ptr world):NetEntity(world)
	{

	}
public:

	ProxyEntity():NetEntity(nullptr)
	{
		eEntityType = EMEntityType::Proxy;
	}

	ProxyEntity(uint32_t id):NetEntity(nullptr)
	{
		eEntityType = EMEntityType::Proxy;
	}

	virtual ~ProxyEntity()
	{
	}

public: // dll override
	/// @brief authenticate token
	std::string& Token() { return sToken; }

	/// @brief authenticate token expire time
	int64_t& ExpireTime() { return iExpireTime; }

	/// @brief alread connected serverid 
	uint32_t& RecordServerId() { return iRecordServerId; }

protected: // dll proxy
	uint32_t iRecordServerId = 0;

	std::string sToken;

	int64_t iExpireTime = 0;

};
