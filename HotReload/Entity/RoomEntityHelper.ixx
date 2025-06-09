module;
export module RoomEntityHelper;

import RoomEntity;

export class RoomEntityHelper : public RoomEntity
{
private:

	RoomEntityHelper() = delete;
	~RoomEntityHelper() = default;

	RoomEntityHelper(const RoomEntityHelper&) = delete;
	void operator=(const RoomEntityHelper&) = delete;

	RoomEntityHelper(RoomEntityHelper&&) = delete;
	RoomEntityHelper& operator=(RoomEntityHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<RoomEntityHelper>;
	using CVPtr = const Ptr&;

	void SetMapID(uint32_t mapId) { iMapId = mapId; }

	std::string ServerIp() { return sServIp; }
	void SetServerIp(const std::string& ip) { sServIp = ip; }

	uint16_t ServerPort() { return iServPort; }
	void SetServerPort(uint16_t port) { iServPort = port; }

	uint32_t ConnNum() { return IConnNum; }

	/// @brief the this close timedown destroy timerid.
	/// @brief authenticate,shutdown and reconnect waiting.
	uint64_t TimerId() { return iCloseTimerId; }
	void SetTimerId(uint64_t timerId) { iCloseTimerId = timerId; }

	/// @brief net socket set
	DNSocketChannel::CVPtr GetChannel() { return pChannel; }

	/// @brief net socket get
	void SetChannel(DNSocketChannel::CVPtr channel) { pChannel = channel; }

};
