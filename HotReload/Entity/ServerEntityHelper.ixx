export module ServerEntityHelper;

import ServerEntity;
import FuncUtils;

export class ServerEntityHelper : public Helper<ServerEntityHelper, ServerEntity>
{
private:

	ServerEntityHelper() = delete;
	~ServerEntityHelper() = default;

public: // dll override

	void SetServerType(EMServerType type) { emServerType = type; }

	void SetLinkNode(ServerEntity::Ptr node) { pLink = node; }

	std::string GetServerIp() { return sServIp; }
	void SetServerIp(const std::string& ip) { sServIp = ip; }

	uint16_t GetServerPort() { return iServPort; }
	void SetServerPort(uint16_t port) { iServPort = port; }

	/// @brief this server connected clients num
	uint32_t GetConnNum() { return IConnNum; }
	void SetConnNum(int div) { IConnNum += div; }

	/// @brief this server child add
	void SetMapLinkNode(EMServerType type, ServerEntity::Ptr node)
	{
		if (type <= EMServerType::None || type >= EMServerType::Max)
		{
			return;
		}

		mMapLink[type].emplace_back(node);
	}

	/// @brief the this close timedown destroy timerid.
	/// @brief authenticate,shutdown and reconnect waiting.
	size_t GetTimerId() { return iCloseTimerId; }

	void SetTimerId(size_t timerId) { iCloseTimerId = timerId; }

	/// @brief net socket set
	const SocketChannel::Ptr& GetChannel() { return pChannel; }

	/// @brief net socket get
	void SetChannel(const SocketChannel::Ptr& channel) { pChannel = channel; }

};
