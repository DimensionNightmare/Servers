module;
export module RoomEntity;

import ECSW;
import DNSocketProxy;

/// @brief room mean set/team/... client collection.
export class RoomEntity : public Entity
{
protected:
	RoomEntity(World::WPtr world):Entity(world)
	{
		eEntityType = EMEntityType::Room;
	}
public:
	using Ptr = std::shared_ptr<RoomEntity>;

	virtual ~RoomEntity() = default;

	uint32_t& MapID() { return iMapId; }

	std::string& ServerIp() { return sServIp; }

	uint16_t& ServerPort() { return iServPort; }

	uint32_t& ConnNum() { return IConnNum; }

	/// @brief the this close timedown destroy timerid.
	/// @brief authenticate,shutdown and reconnect waiting.
	uint64_t& TimerId() { return iCloseTimerId; }

	/// @brief net socket set
	const DNSocketProxy::Ptr& GetSock() { return pSock; }

	/// @brief net socket get
	void SetSock(const DNSocketProxy::Ptr& sock) { pSock = sock; }

public: // dll override

protected: // dll proxy

	uint32_t iMapId = 0;

	std::string sServIp;

	uint16_t iServPort = 0;

	uint32_t IConnNum = 0;

	uint64_t iCloseTimerId = 0;

	DNSocketProxy::Ptr pSock;
	
};
