module;
#include <unordered_map>
#include <list>
#include <atomic>
#include <functional>
#include <mutex>
#include <shared_mutex>

#include "StdMacro.h"
export module RoomEntityManager;

export import RoomEntity;
import EntityManager;
import Logger;

export class RoomEntityManager : public EntityManager<RoomEntity>
{
public:
	RoomEntityManager() = default;
	~RoomEntityManager() = default;

	virtual bool Init() override;

	virtual void TickMainFrame() override;

public:
	void EntityCloseTimer(uint64_t timerID);

	uint64_t CheckEntityCloseTimer(uint32_t entityId);

	bool RemoveEntity(uint32_t entityId);
protected:
	unordered_map<uint32_t, list<RoomEntity*>> mEntityMapList;

	atomic<uint32_t> iRoomGenId;
};

extern "C"
{
	REGIST_MAINSPACE_SIGN_FUNCTION(RoomEntityManager, CheckEntityCloseTimer);
}

bool RoomEntityManager::Init()
{
	return EntityManager::Init();
}

void RoomEntityManager::TickMainFrame()
{
}


void RoomEntityManager::EntityCloseTimer(uint64_t timerID)
{
	unique_lock<shared_mutex> ulock(oTimerMutex);
	if (!mMapTimer.contains(timerID))
	{
		return;
	}

	uint32_t entityId = mMapTimer[timerID];
	if (RemoveEntity(entityId))
	{
		DNPrint(0, LoggerLevel::Debug, "EntityCloseTimer server destory entity");
	}

}

uint64_t RoomEntityManager::CheckEntityCloseTimer(uint32_t entityId)
{
	uint64_t timerId = Timer()->setTimeout(10000, std::bind(&RoomEntityManager::EntityCloseTimer, this, placeholders::_1));

	AddTimerRecord(timerId, entityId);

	return timerId;
}

bool RoomEntityManager::RemoveEntity(uint32_t entityId)
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
