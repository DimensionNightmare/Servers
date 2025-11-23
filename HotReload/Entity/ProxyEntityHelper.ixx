export module ProxyEntityHelper;

import ProxyEntity;
import FuncUtils;

export class ProxyEntityHelper : public Helper<ProxyEntityHelper, ProxyEntity>
{
private:

	ProxyEntityHelper() = delete;
	~ProxyEntityHelper() = default;

public: 

	/// @brief the this close timedown destroy timerid.
	/// @brief authenticate,shutdown and reconnect waiting.
	size_t GetTimerId() { return iCloseTimerId; }
	void SetTimerId(size_t timerId) { iCloseTimerId = timerId; }

	/// @brief net socket set
	SocketChannel::Ptr GetChannel() { return pChannel; }

	/// @brief net socket get
	void SetChannel(SocketChannel::CVPtr channel) { pChannel = channel; }

	/// @brief authenticate token
	std::string GetToken() { return sToken; }
	void SetToken(const std::string& token) { sToken = token; }

	/// @brief authenticate token expire time
	int64_t GetExpireTime() { return iExpireTime; }
	void SetExpireTime(int64_t time) { iExpireTime = time; }

	/// @brief alread connected serverid 
	size_t GetRecordServerId() { return iRecordServerId; }
	void SetRecordServerId(size_t id) { iRecordServerId = id; }
};
