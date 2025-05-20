module;
export module RoomEntity;

import ECSW;
import ThirdParty.Libhv;

/// @brief room mean set/team/... client collection.
export class RoomEntity : public Entity
{
protected:
	friend class RoomEntityManagerHelper;
	RoomEntity(World::WPtr world):Entity(world)
	{
		eEntityType = EMEntityType::Room;
	}
public:
	using Ptr = std::shared_ptr<RoomEntity>;

	virtual ~RoomEntity() = default;

	uint32_t MapID() { return iMapId; }

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
	const DNSocketChannel::Ptr& GetChannel() { return pChannel; }

	/// @brief net socket get
	void SetChannel(const DNSocketChannel::Ptr& channel) { pChannel = channel; }

public: // dll override

protected: // dll proxy

	uint32_t iMapId = 0;

	std::string sServIp;

	uint16_t iServPort = 0;

	uint32_t IConnNum = 0;

	uint64_t iCloseTimerId = 0;

	DNSocketChannel::Ptr pChannel;
	
};
