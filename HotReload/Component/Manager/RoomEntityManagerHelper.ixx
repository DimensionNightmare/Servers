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
		RoomEntity::CVPtr entity = Base()->AddEntity(entityId);
		mEntityMapList[entityId].emplace_back(entity);

		RoomEntityHelper::CVPtr helper = entity->GetSelf<RoomEntityHelper>();
		helper->SetMapID(entityId);
		return helper;
	}

	void MountEntity(RoomEntityHelper::CVPtr entity)
	{
		std::unique_lock ulock(oMapMutex);
		if (mEntityMap.contains(entity->ID()))
		{
			mEntityMapList[entity->MapID()].emplace_back(entity);
		}
	}

	void UnMountEntity(RoomEntityHelper::CVPtr entity)
	{
		std::unique_lock ulock(oMapMutex);
		mEntityMapList[entity->MapID()].remove(entity);
	}

	RoomEntityHelper::Ptr GetEntity(uint64_t entityId)
	{
		std::shared_lock lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return mEntityMap[entityId]->GetSelf<RoomEntityHelper>();
		}
		// allow return empty
		return nullptr;
	}

	const std::list<RoomEntity::Ptr>& GetEntitysByMapId(uint64_t mapId)
	{
		std::shared_lock lock(oMapMutex);
		return mEntityMapList[mapId];
	}

	[[nodiscard]] uint64_t GenRoomId()
	{
		return ++iRoomGenId;
	}
};
