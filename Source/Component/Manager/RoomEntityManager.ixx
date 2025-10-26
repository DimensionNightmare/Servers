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

	void EntityCloseTimer(size_t timerID)
	{
		std::unique_lock ulock(oTimerMutex);
		if (!mMapTimer.contains(timerID))
		{
			return;
		}

		size_t entityId = mMapTimer[timerID];

		mMapTimer.erase(timerID);

		if (RemoveEntity(entityId))
		{
			LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "EntityCloseTimer Room destory entity");
		}

	}

public: // dll proxy

	bool RemoveEntity(size_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			RoomEntity::Ptr entity = mEntityMap[entityId];
			entity->Dispose();
			
			std::unique_lock ulock(oMapMutex);
			mEntityMapList[entity->MapID()].remove(entity);
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

protected:

	RoomEntity::Ptr _AddEntity(size_t mapId)
	{
		RoomEntity::Ptr entity = P_InstanceHolder->GetMemPool().Allocate<RoomEntity>(GetOwner()->GetWorldW());

		std::unique_lock ulock(oMapMutex);
		mEntityMap[entity->ID()] = entity;
		return entity;
	}

	size_t _CheckEntityCloseTimer(size_t entityId)
	{
		FunctionContainer<&RoomEntityManager::EntityCloseTimer> funcProxy(this);

		size_t timerId = GetTimer()->SetTimeout(10000, funcProxy);

		AddTimerRecord(timerId, entityId);

		return timerId;
	}

public:
	FunctionContainer<&RoomEntityManager::_AddEntity> AddEntity;
	FunctionContainer<&RoomEntityManager::_CheckEntityCloseTimer> CheckEntityCloseTimer;

protected:
	/// @brief 
	std::unordered_map<size_t, std::list<RoomEntity::Ptr>> mEntityMapList;

	/// @brief 
	std::atomic<size_t> iRoomGenId;

};
