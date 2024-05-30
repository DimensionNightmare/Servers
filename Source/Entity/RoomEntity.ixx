module;
export module RoomEntity;

import Entity;

export class RoomEntity : public Entity
{
public:
	RoomEntity();
	RoomEntity(uint32_t id);
	virtual ~RoomEntity() = default;

public: // dll override

protected: // dll proxy
};

RoomEntity::RoomEntity()
{
	eEntityType = EntityType::Room;
}

RoomEntity::RoomEntity(uint32_t id) : Entity(id)
{
	eEntityType = EntityType::Room;
}
