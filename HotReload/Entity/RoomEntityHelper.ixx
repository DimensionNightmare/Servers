export module RoomEntityHelper;

import RoomEntity;
import FuncUtils;

export class RoomEntityHelper : public Helper<RoomEntityHelper, RoomEntity>
{
private:

	RoomEntityHelper() = delete;
	~RoomEntityHelper() = default;

public:

	void SetMapID(uint32_t mapId) { iMapId = mapId; }

	std::string ServerIp() { return sServIp; }
	void SetServerIp(const std::string& ip) { sServIp = ip; }

	uint16_t ServerPort() { return iServPort; }
	void SetServerPort(uint16_t port) { iServPort = port; }

	uint32_t ConnNum() { return IConnNum; }

	/// @brief the this close timedown destroy timerid.
	/// @brief authenticate,shutdown and reconnect waiting.
	size_t TimerId() { return iCloseTimerId; }
	void SetTimerId(size_t timerId) { iCloseTimerId = timerId; }

	/// @brief net socket set
	SocketChannel::Ptr GetChannel() { return pChannel; }

	/// @brief net socket get
	void SetChannel(SocketChannel::Ptr channel) { pChannel = channel; }

};
