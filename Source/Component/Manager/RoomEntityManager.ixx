export module RoomEntityManager;

import RoomEntity;
import EntityManager;
import FuncUtils;

export class RoomEntityManager : public EntityManager<RoomEntity>
{
protected:
	friend class System;
	friend class UniversalMemoryPool;
	/// @brief timer manager create
	RoomEntityManager(System::WPtr system):EntityManager(system)
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
		EntityManager::Dispose();

		mEntityMapList.clear();
	}

	virtual void TickMainFrame() override
	{
	}

	void EntityCloseTimer(uint64_t timerID)
	{
		std::unique_lock ulock(oTimerMutex);
		if (!mMapTimer.contains(timerID))
		{
			return;
		}

		uint64_t entityId = mMapTimer[timerID];

		mMapTimer.erase(timerID);

		if (RemoveEntity(entityId))
		{
			GetLogger()->Record(ELogLevel_Debug, "EntityCloseTimer Room destory entity");
		}

	}

public: // dll proxy

	bool RemoveEntity(uint64_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			RoomEntity::CVPtr entity = mEntityMap[entityId];
			entity->Dispose();
			
			std::unique_lock ulock(oMapMutex);
			mEntityMapList[entity->MapID()].remove(entity);
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

	RoomEntity::Ptr _AddEntity(uint64_t mapId)
	{
		RoomEntity::Ptr entity = MemPool->Allocate<RoomEntity, World::WPtr>(GetOwner()->GetWorldW());

		std::unique_lock ulock(oMapMutex);
		mEntityMap[entity->ID()] = entity;
		return entity;
	}

	uint64_t _CheckEntityCloseTimer(uint64_t entityId)
	{
		FunctionContainer<&RoomEntityManager::EntityCloseTimer> funcProxy(this);

		uint64_t timerId = GetTimer()->SetTimeout(10000, funcProxy);

		AddTimerRecord(timerId, entityId);

		return timerId;
	}

public:
	FunctionContainer<&RoomEntityManager::_AddEntity> AddEntity;
	FunctionContainer<&RoomEntityManager::_CheckEntityCloseTimer> CheckEntityCloseTimer;

protected:
	/// @brief 
	std::unordered_map<uint64_t, std::list<RoomEntity::Ptr>> mEntityMapList;

	/// @brief 
	std::atomic<uint64_t> iRoomGenId;

};
