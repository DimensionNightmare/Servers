export module ProxyEntityHelper;

import ProxyEntity;
import FuncUtils;

export class ProxyEntityHelper : public Helper<ProxyEntityHelper, ProxyEntity>
{
private:

	ProxyEntityHelper() = delete;
	~ProxyEntityHelper() = default;

public: // dll override

	/// @brief the this close timedown destroy timerid.
	/// @brief authenticate,shutdown and reconnect waiting.
	uint64_t TimerId() { return iCloseTimerId; }
	void SetTimerId(uint64_t timerId) { iCloseTimerId = timerId; }

	/// @brief net socket set
	SocketChannel::CVPtr GetChannel() { return pChannel; }

	/// @brief net socket get
	void SetChannel(SocketChannel::CVPtr channel) { pChannel = channel; }

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
