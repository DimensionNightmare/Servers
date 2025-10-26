export module RoomEntityManagerHelper;

import RoomEntityManager;
import RoomEntityHelper;
import FuncUtils;

export class RoomEntityManagerHelper : public Helper<RoomEntityManagerHelper, RoomEntityManager>
{

private:

	RoomEntityManagerHelper() = delete;
	~RoomEntityManagerHelper() = default;

public:

	RoomEntityHelper::Ptr AddEntity(uint32_t entityId)
	{
		RoomEntity::Ptr entity = Base()->AddEntity(entityId);
		mEntityMapList[entityId].emplace_back(entity);

		RoomEntityHelper::Ptr helper = entity->GetSelf<RoomEntityHelper>();
		helper->SetMapID(entityId);
		return helper;
	}

	void MountEntity(RoomEntityHelper::Ptr entity)
	{
		std::unique_lock ulock(oMapMutex);
		if (mEntityMap.contains(entity->ID()))
		{
			mEntityMapList[entity->MapID()].emplace_back(entity);
		}
	}

	void UnMountEntity(RoomEntityHelper::Ptr entity)
	{
		std::unique_lock ulock(oMapMutex);
		mEntityMapList[entity->MapID()].remove(entity);
	}

	RoomEntityHelper::Ptr GetEntity(size_t entityId)
	{
		std::shared_lock lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return mEntityMap[entityId]->GetSelf<RoomEntityHelper>();
		}
		// allow return empty
		return nullptr;
	}

	const std::list<RoomEntity::Ptr>& GetEntitysByMapId(size_t mapId)
	{
		std::shared_lock lock(oMapMutex);
		return mEntityMapList[mapId];
	}

	[[nodiscard]] size_t GenRoomId()
	{
		return ++iRoomGenId;
	}
};
