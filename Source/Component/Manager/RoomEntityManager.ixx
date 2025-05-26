module;
export module RoomEntityManager;

import RoomEntity;
import EntityManager;

export class RoomEntityManager : public EntityManager<RoomEntity>
{
protected:
	friend class System;
	/// @brief timer manager create
	RoomEntityManager(System::WPtr system):EntityManager(system)
	{
		eComponentType = EMComponentType::RoomEntityManager;
	}
public:
	~RoomEntityManager() = default;

	virtual void Dispose() override
	{
		EntityManager::Dispose();
	}

	virtual void TickMainFrame() override
	{
	}

public:

	void EntityCloseTimer(uint64_t timerID)
	{
		std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
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

	bool RemoveEntity(uint64_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			RoomEntity::Ptr entity = mEntityMap[entityId];
			std::unique_lock<std::shared_mutex> ulock(oMapMutex);

			mEntityMapList[entity->MapID()].remove(entity);

			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

	void AddEntity(uint64_t entityId)
	{
		RoomEntity::Ptr entity = std::shared_ptr<RoomEntity>(new RoomEntity(GetOwner()->GetWorldW()));
		entity->SetID(entityId);

		std::unique_lock<std::shared_mutex> ulock(oMapMutex);
		mEntityMap[entityId] = entity;
	}

protected:
	/// @brief 
	std::unordered_map<uint64_t, std::list<RoomEntity::Ptr>> mEntityMapList;

	/// @brief 
	std::atomic<uint64_t> iRoomGenId;

};
