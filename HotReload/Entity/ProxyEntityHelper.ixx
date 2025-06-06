module;
export module ProxyEntityHelper;

import ProxyEntity;

export class ProxyEntityHelper : public ProxyEntity
{
private:

	ProxyEntityHelper() = delete;
	~ProxyEntityHelper() = default;

	ProxyEntityHelper(const ProxyEntityHelper&) = delete;
	void operator=(const ProxyEntityHelper&) = delete;

	ProxyEntityHelper(ProxyEntityHelper&&) = delete;
	ProxyEntityHelper& operator=(ProxyEntityHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public: // dll override
	using Ptr = std::shared_ptr<ProxyEntityHelper>;

	/// @brief the this close timedown destroy timerid.
	/// @brief authenticate,shutdown and reconnect waiting.
	uint64_t TimerId() { return iCloseTimerId; }
	void SetTimerId(uint64_t timerId) { iCloseTimerId = timerId; }

	/// @brief net socket set
	const DNSocketChannel::Ptr& GetChannel() { return pChannel; }

	/// @brief net socket get
	void SetChannel(const DNSocketChannel::Ptr& channel) { pChannel = channel; }

	/// @brief authenticate token
	std::string Token() { return sToken; }
	void SetToken(const std::string& token) { sToken = token; }

	/// @brief authenticate token expire time
	int64_t ExpireTime() { return iExpireTime; }
	void SetExpireTime(int64_t time) { iExpireTime = time; }

	/// @brief alread connected serverid 
	uint64_t RecordServerId() { return iRecordServerId; }
	void SetRecordServerId(uint64_t id) { iRecordServerId = id; }
};
