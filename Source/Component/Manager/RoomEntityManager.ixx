export module RoomEntityManager;

import RoomEntity;
import EntityManager;
import FuncUtils;
import Logger;

export class RoomEntityManager : public EntityManager<RoomEntity>
{
protected:

	friend class UniversalMemoryPool;
	/// @brief timer manager create
	RoomEntityManager(System::CVPtr system):EntityManager(system)
		,CheckEntityCloseTimer(this)
		,AddEntity(this)
	{
		eComponentType = EMComponentType::RoomEntityManager;
	}
public:
	virtual ~RoomEntityManager()
	{
		
	}

	virtual void Dispose() override
	{
		mEntityMapList.clear();
		
		EntityManager::Dispose();
	}

	void EntityCloseTimer(size_t timerID)
	{
		std::unique_lock ulock(oTimerMutex);
		if (!mMapTimer.contains(timerID))
		{
			return;
		}

		size_t entityId = mMapTimer[timerID];

		mMapTimer.erase(timerID);

		if (auto entity = RemoveEntity(entityId))
		{
			entity->Dispose();
			LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "EntityCloseTimer Room destory entity {}", entity->ID());
		}

	}

protected:

	RoomEntity::Ptr RemoveEntity(size_t entityId)
	{
		RoomEntity::Ptr entity;

		if (mEntityMap.contains(entityId))
		{
			
			std::unique_lock ulock(oMapMutex);
			entity = std::move(mEntityMap[entityId]);
			mEntityMapList[entity->MapID()].remove(entity);
			mEntityMap.erase(entityId);
		}

		return entity;
	}

	RoomEntity::Ptr _AddEntity(size_t mapId)
	{
		RoomEntity::Ptr entity = P_InstanceHolder->GetMemPool().Allocate<RoomEntity>(GetWorld());

		std::unique_lock ulock(oMapMutex);
		mEntityMap[entity->ID()] = entity;
		return entity;
	}

	size_t _CheckEntityCloseTimer(size_t entityId)
	{
		EventContainer<&RoomEntityManager::EntityCloseTimer> funcProxy(this);

		size_t timerId = GetTimer()->SetTimeout(10000, funcProxy);

		AddTimerRecord(timerId, entityId);

		return timerId;
	}

public:
	EventContainer<&RoomEntityManager::_AddEntity> AddEntity;
	EventContainer<&RoomEntityManager::_CheckEntityCloseTimer> CheckEntityCloseTimer;

protected:
	/// @brief 
	std::unordered_map<size_t, std::list<RoomEntity::Ptr>> mEntityMapList;

	/// @brief 
	std::atomic<size_t> iRoomGenId;

};
