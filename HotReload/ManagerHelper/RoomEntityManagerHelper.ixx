module;
#include "StdMacro.h"
export module RoomEntityManagerHelper;

import RoomEntityManager;
import RoomEntityHelper;
import Logger;

export class RoomEntityManagerHelper : public RoomEntityManager
{

private:

	RoomEntityManagerHelper() {}
public:

	RoomEntity* AddEntity(uint32_t entityId, uint32_t mapId)
	{
		if (!mEntityMap.contains(entityId))
		{
			unique_lock<shared_mutex> ulock(oMapMutex);

			mEntityMap.emplace(std::piecewise_construct,
				std::forward_as_tuple(entityId),
				std::forward_as_tuple(entityId));

			RoomEntity* entity = &mEntityMap[entityId];

			entity->MapID() = mapId;

			mEntityMapList[mapId].emplace_back(entity);
			return entity;
		}

		return nullptr;
	}

	bool RemoveEntity(uint32_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			RoomEntity* entity = &mEntityMap[entityId];

			unique_lock<shared_mutex> ulock(oMapMutex);

			mEntityMapList[entity->MapID()].remove(entity);

			DNPrint(0, EMLoggerLevel::Debug, "offline destory entity");
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

	void MountEntity(RoomEntity* entity)
	{
		unique_lock<shared_mutex> ulock(oMapMutex);
		if (mEntityMap.contains(entity->ID()))
		{
			mEntityMapList[entity->MapID()].emplace_back(entity);
		}
	}

	void UnMountEntity(RoomEntity* entity)
	{
		unique_lock<shared_mutex> ulock(oMapMutex);
		mEntityMapList[entity->MapID()].remove(entity);
	}

	RoomEntity* GetEntity(uint32_t entityId)
	{
		shared_lock<shared_mutex> lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return &mEntityMap[entityId];
		}
		// allow return empty
		return nullptr;
	}

	const list<RoomEntity*>& GetEntitysByMapId(uint32_t mapId)
	{
		shared_lock<shared_mutex> lock(oMapMutex);
		return mEntityMapList[mapId];
	}

	[[nodiscard]] uint32_t GenRoomId()
	{
		return ++iRoomGenId;
	}
};
