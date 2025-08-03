export module RoomEntity;

import ECSW;
import std.compat;
import ThirdParty.Libhv;

/// @brief room mean set/team/... client collection.
export class RoomEntity : public Entity
{
protected:
	friend class RoomEntityManager;
	friend class UniversalMemoryPool;
	RoomEntity(World::WPtr world):Entity(world)
	{
		eEntityType = EMEntityType::Room;
	}
public:
	using Ptr = std::shared_ptr<RoomEntity>;
	using CVPtr = const Ptr&;

	virtual ~RoomEntity() = default;

public: // dll override

	uint32_t MapID() { return iMapId; }

protected: // dll proxy

	uint32_t iMapId = 0;

	std::string sServIp;

	uint16_t iServPort = 0;

	uint32_t IConnNum = 0;

	uint64_t iCloseTimerId = 0;

	SocketChannel::Ptr pChannel;
	
};
