module;
export module ProxyEntity;

import ECSW;
import DNSocketProxy;

/// @brief this is client proxy entity
export class ProxyEntity : public Entity
{
protected:
	friend class ProxyEntityManagerHelper;
	ProxyEntity(World::WPtr world):Entity(world)
	{
		eEntityType = EMEntityType::Proxy;
	}
public:
	using Ptr = std::shared_ptr<ProxyEntity>;
	virtual ~ProxyEntity()
	{
	}

	/// @brief the this close timedown destroy timerid.
	/// @brief authenticate,shutdown and reconnect waiting.
	uint64_t TimerId() { return iCloseTimerId; }
	void SetTimerId(uint64_t timerId) { iCloseTimerId = timerId; }

	/// @brief net socket set
	const DNSocketProxy::Ptr& GetSock() { return pSock; }

	/// @brief net socket get
	void SetSock(const DNSocketProxy::Ptr& sock) { pSock = sock; }

public: // dll override
	/// @brief authenticate token
	std::string Token() { return sToken; }
	void SetToken(const std::string& token) { sToken = token; }

	/// @brief authenticate token expire time
	int64_t ExpireTime() { return iExpireTime; }
	void SetExpireTime(int64_t time) { iExpireTime = time; }

	/// @brief alread connected serverid 
	uint64_t RecordServerId() { return iRecordServerId; }
	void SetRecordServerId(uint64_t id) { iRecordServerId = id; }

protected: // dll proxy
	uint64_t iRecordServerId = 0;

	std::string sToken;

	int64_t iExpireTime = 0;

	uint64_t iCloseTimerId = 0;

	DNSocketProxy::Ptr pSock;

};
