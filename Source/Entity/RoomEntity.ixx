module;
#include "StdMacro.h"
export module RoomEntity;

import NetEntity;

export class RoomEntity : public NetEntity
{
public:
	RoomEntity();
	RoomEntity(uint32_t id);
	virtual ~RoomEntity() = default;

	uint32_t& MapID() { return iMapId; }

	string& ServerIp() { return sServIp; }

	uint16_t& ServerPort() { return iServPort; }

	uint32_t& ConnNum() { return IConnNum; }

public: // dll override

protected: // dll proxy
	uint32_t iMapId = 0;

	string sServIp;
	uint16_t iServPort = 0;
	uint32_t IConnNum = 0;
};

RoomEntity::RoomEntity() : NetEntity(0)
{
	eEntityType = EntityType::Room;
}

RoomEntity::RoomEntity(uint32_t id) : NetEntity(id)
{
	eEntityType = EntityType::Room;
}
