export module RoomEntityManager;

import RoomEntity;
import EntityManager;

export class RoomEntityManager : public EntityManager<RoomEntity>
{
protected:
	friend class System;
	friend class UniversalMemoryPool;
	/// @brief timer manager create
	RoomEntityManager(System::WPtr system):EntityManager(system)
	{
		eComponentType = EMComponentType::RoomEntityManager;

		pCheckEntityCloseTimer = std::bind(&RoomEntityManager::CheckEntityCloseTimer, this, std::placeholders::_1);
		pAddEntity = std::bind(&RoomEntityManager::AddEntity, this, std::placeholders::_1);
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
		if (RemoveEntity(entityId))
		{
			GetLogger()->Record(ELogLevel_Debug, "EntityCloseTimer Room destory entity");
		}

	}

	uint64_t CheckEntityCloseTimer(uint64_t entityId)
	{
		uint64_t timerId = Timer()->setTimeout(10000, std::bind(&RoomEntityManager::EntityCloseTimer, this, std::placeholders::_1));

		AddTimerRecord(timerId, entityId);

		return timerId;
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

	RoomEntity::Ptr AddEntity(uint64_t mapId)
	{
		// RoomEntity::CVPtr entity = std::shared_ptr<RoomEntity>(new RoomEntity(GetOwner()->GetWorldW()));
		RoomEntity::Ptr entity = MemPool->Allocate<RoomEntity, World::WPtr>(GetOwner()->GetWorldW());
		// entity->SetID(entityId);

		std::unique_lock ulock(oMapMutex);
		mEntityMap[entity->ID()] = entity;
		mEntityMapList[mapId].emplace_back(entity);
		return entity;
	}

public:
	std::function<RoomEntity::Ptr(uint64_t)> pAddEntity;

protected:
	/// @brief 
	std::unordered_map<uint64_t, std::list<RoomEntity::Ptr>> mEntityMapList;

	/// @brief 
	std::atomic<uint64_t> iRoomGenId;

};
