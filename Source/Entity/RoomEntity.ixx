module;
export module RoomEntity;

import ECSW;

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

public: // dll override

protected: // dll proxy

	uint32_t iMapId = 0;

	std::string sServIp;

	uint16_t iServPort = 0;

	uint32_t IConnNum = 0;
	
};
