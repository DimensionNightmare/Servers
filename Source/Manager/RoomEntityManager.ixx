module;
#include "StdMacro.h"
export module RoomEntityManager;

import RoomEntity;
import EntityManager;
import Logger;

export class RoomEntityManager : public EntityManager<RoomEntity>
{
	
public:
	RoomEntityManager() = default;
	~RoomEntityManager() = default;

	virtual bool Init() override
	{
		return EntityManager::Init();
	}

	virtual void TickMainFrame() override
	{
	}

public:

	void EntityCloseTimer(uint64_t timerID)
	{
		unique_lock<shared_mutex> ulock(oTimerMutex);
		if (!mMapTimer.contains(timerID))
		{
			return;
		}

		uint32_t entityId = mMapTimer[timerID];
		if (RemoveEntity(entityId))
		{
			DNPrint(0, EMLoggerLevel::Debug, "EntityCloseTimer Room destory entity");
		}

	}

	uint64_t CheckEntityCloseTimer(uint32_t entityId)
	{
		uint64_t timerId = Timer()->setTimeout(10000, std::bind(&RoomEntityManager::EntityCloseTimer, this, placeholders::_1));

		AddTimerRecord(timerId, entityId);

		return timerId;
	}

	bool RemoveEntity(uint32_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			RoomEntity* entity = &mEntityMap[entityId];
			unique_lock<shared_mutex> ulock(oMapMutex);

			mEntityMapList[entity->MapID()].remove(entity);

			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

protected:
	/// @brief 
	unordered_map<uint32_t, list<RoomEntity*>> mEntityMapList;

	/// @brief 
	atomic<uint32_t> iRoomGenId;

};
