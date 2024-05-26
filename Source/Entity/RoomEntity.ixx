module;
export module RoomEntity;

import Entity;

export class RoomEntity : public Entity
{
public:
	RoomEntity();
	virtual ~RoomEntity() = default;

public: // dll override

protected: // dll proxy
};

RoomEntity::RoomEntity()
{
	eEntityType = EntityType::Room;

}
