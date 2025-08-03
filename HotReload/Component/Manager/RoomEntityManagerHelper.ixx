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
	using CVPtr = const Ptr&;

	RoomEntityHelper::Ptr AddEntity(uint32_t mapId)
	{
		uint64_t entityId = 0;
		TickMainSpaceDll(this, FUNCPLACE(RoomEntityManager,AddEntity), std::ref(entityId), mapId);
		RoomEntityHelper::CVPtr entity = GetEntity(entityId);
		entity->SetMapID(mapId);
		return entity;
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
