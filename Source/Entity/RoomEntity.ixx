export module RoomEntity;

import ECSW;
import std.compat;
import ThirdParty.Libhv;

/// @brief room mean set/team/... client collection.
export class RoomEntity : public Entity
{
protected:
	friend class UniversalMemoryPool;
	RoomEntity(World::CVPtr world):Entity(world)
	{
		eEntityType = EMEntityType::Room;
	}
public:
	using Ptr = std::shared_ptr<RoomEntity>;
	using CVPtr = const Ptr&;

	virtual ~RoomEntity()
	{
		
	}

public: 

	size_t MapID() { return iMapId; }

protected: // dll proxy

	size_t iMapId = 0;

	std::string sServIp;

	uint16_t iServPort = 0;

	uint32_t IConnNum = 0;

	size_t iCloseTimerId = 0;

	SocketChannel::Ptr pChannel;
	
};
