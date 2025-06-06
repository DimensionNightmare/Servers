module;
export module ServerEntityHelper;

import ServerEntity;

export class ServerEntityHelper : public ServerEntity
{
private:

	ServerEntityHelper() = delete;
	~ServerEntityHelper() = default;

	ServerEntityHelper(const ServerEntityHelper&) = delete;
	void operator=(const ServerEntityHelper&) = delete;

	ServerEntityHelper(ServerEntityHelper&&) = delete;
	ServerEntityHelper& operator=(ServerEntityHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public: // dll override

	using Ptr = std::shared_ptr<ServerEntityHelper>;

	void SetServerType(EMServerType type) { emServerType = type; }

	void SetLinkNode(const ServerEntity::Ptr& node) { pLink = node; }

	std::string ServerIp() { return sServIp; }
	void SetServerIp(const std::string& ip) { sServIp = ip; }

	uint16_t ServerPort() { return iServPort; }
	void SetServerPort(uint16_t port) { iServPort = port; }

	/// @brief this server connected clients num
	uint32_t ConnNum() { return IConnNum; }
	void SetConnNum(int div) { IConnNum += div; }

	/// @brief this server child add
	void SetMapLinkNode(EMServerType type, const ServerEntity::Ptr& node)
	{
		if (type <= EMServerType::None || type >= EMServerType::Max)
		{
			return;
		}

		mMapLink[type].emplace_back(node);
	}

	/// @brief the this close timedown destroy timerid.
	/// @brief authenticate,shutdown and reconnect waiting.
	uint64_t TimerId() { return iCloseTimerId; }

	void SetTimerId(uint64_t timerId) { iCloseTimerId = timerId; }

	/// @brief net socket set
	const DNSocketChannel::Ptr& GetChannel() { return pChannel; }

	/// @brief net socket get
	void SetChannel(const DNSocketChannel::Ptr& channel) { pChannel = channel; }

};
