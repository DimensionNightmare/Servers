module;
export module RoomEntity;


import NetEntity;

/// @brief room mean set/team/... client collection.
export class RoomEntity : public NetEntity
{
protected:
	RoomEntity(World::Ptr world):NetEntity(world)
	{

	}
public:

	RoomEntity():NetEntity(nullptr)
	{
		eEntityType = EMEntityType::Room;
	}

	RoomEntity(uint32_t id):NetEntity(nullptr)
	{
		eEntityType = EMEntityType::Room;
	}

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
