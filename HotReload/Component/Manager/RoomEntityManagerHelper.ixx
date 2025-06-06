module;
export module RoomEntityManagerHelper;

import RoomEntityManager;
import DllUtils;
import RoomEntityHelper;

#define FUNCPLACE(class, func) &class::func, #class"_"#func


export class RoomEntityManagerHelper : public RoomEntityManager
{

private:

	RoomEntityManagerHelper() = delete;
	~RoomEntityManagerHelper() = default;

	RoomEntityManagerHelper(const RoomEntityManagerHelper&) = delete;
	void operator=(const RoomEntityManagerHelper&) = delete;

	RoomEntityManagerHelper(RoomEntityManagerHelper&&) = delete;
	RoomEntityManagerHelper& operator=(RoomEntityManagerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<RoomEntityManagerHelper>;

	RoomEntityHelper::Ptr AddEntity(uint64_t entityId, uint32_t mapId)
	{
		if (!mEntityMap.contains(entityId))
		{
			TickMainSpaceDll(this, FUNCPLACE(RoomEntityManager,AddEntity), entityId, mapId);
			RoomEntityHelper::Ptr entity = GetEntity(entityId);
			entity->SetMapID(mapId);
			return entity;
		}

		return nullptr;
	}

	void MountEntity(RoomEntityHelper::Ptr entity)
	{
		std::unique_lock<std::shared_mutex> ulock(oMapMutex);
		if (mEntityMap.contains(entity->ID()))
		{
			mEntityMapList[entity->MapID()].emplace_back(entity);
		}
	}

	void UnMountEntity(RoomEntityHelper::Ptr entity)
	{
		std::unique_lock<std::shared_mutex> ulock(oMapMutex);
		mEntityMapList[entity->MapID()].remove(entity);
	}

	RoomEntityHelper::Ptr GetEntity(uint64_t entityId)
	{
		std::shared_lock<std::shared_mutex> lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return mEntityMap[entityId]->GetSelf<RoomEntityHelper>();
		}
		// allow return empty
		return nullptr;
	}

	const std::list<RoomEntity::Ptr>& GetEntitysByMapId(uint64_t mapId)
	{
		std::shared_lock<std::shared_mutex> lock(oMapMutex);
		return mEntityMapList[mapId];
	}

	[[nodiscard]] uint64_t GenRoomId()
	{
		return ++iRoomGenId;
	}
};
