module;
export module RoomEntityManagerHelper;

import RoomEntityManager;
import RoomEntityHelper;
import Logger;

export class RoomEntityManagerHelper : public RoomEntityManager
{

private:

	RoomEntityManagerHelper() = delete;
	// RoomEntityManagerHelper(System::WPtr):RoomEntityManager(nullptr) {}
public:

	RoomEntity::Ptr AddEntity(uint32_t entityId, uint32_t mapId)
	{
		if (!mEntityMap.contains(entityId))
		{
			std::unique_lock<std::shared_mutex> ulock(oMapMutex);

			mEntityMap.emplace(std::piecewise_construct,
				std::forward_as_tuple(entityId),
				std::forward_as_tuple(entityId));

			RoomEntity::Ptr entity = mEntityMap[entityId];

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
			RoomEntity::Ptr entity = mEntityMap[entityId];

			std::unique_lock<std::shared_mutex> ulock(oMapMutex);

			mEntityMapList[entity->MapID()].remove(entity);

			SPidLogger.Record(ELogLevel_Debug, "offline destory entity");
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

	void MountEntity(RoomEntity::Ptr entity)
	{
		std::unique_lock<std::shared_mutex> ulock(oMapMutex);
		if (mEntityMap.contains(entity->ID()))
		{
			mEntityMapList[entity->MapID()].emplace_back(entity);
		}
	}

	void UnMountEntity(RoomEntity::Ptr entity)
	{
		std::unique_lock<std::shared_mutex> ulock(oMapMutex);
		mEntityMapList[entity->MapID()].remove(entity);
	}

	RoomEntity::Ptr GetEntity(uint32_t entityId)
	{
		std::shared_lock<std::shared_mutex> lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return mEntityMap[entityId];
		}
		// allow return empty
		return nullptr;
	}

	const std::list<RoomEntity::Ptr>& GetEntitysByMapId(uint32_t mapId)
	{
		std::shared_lock<std::shared_mutex> lock(oMapMutex);
		return mEntityMapList[mapId];
	}

	[[nodiscard]] uint32_t GenRoomId()
	{
		return ++iRoomGenId;
	}
};
