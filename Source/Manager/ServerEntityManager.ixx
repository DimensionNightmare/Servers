module;
#include <unordered_map>
#include <list>
#include <shared_mutex>
#include <cstdint>
#include <functional>

#include "StdMacro.h"
export module ServerEntityManager;

export import ServerEntity;
import EntityManager;
import Logger;
import Macro;

using namespace std;

export class ServerEntityManager : public EntityManager<ServerEntity>
{
public:
	ServerEntityManager() = default;

	virtual ~ServerEntityManager() = default;

	virtual bool Init() override;

public: // dll override
	void EntityCloseTimer(uint64_t timerID);

	uint64_t CheckEntityCloseTimer(uint32_t entityId);

	bool RemoveEntity(uint32_t entityId);

protected: // dll proxy
	unordered_map<ServerType, list<ServerEntity*> > mEntityMapList;
	// server pull server
	atomic<uint32_t> iServerId;

};

extern "C"
{
	REGIST_MAINSPACE_SIGN_FUNCTION(ServerEntityManager, CheckEntityCloseTimer);
}

bool ServerEntityManager::Init()
{
	return EntityManager::Init();
}

void ServerEntityManager::EntityCloseTimer(uint64_t timerID)
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

uint64_t ServerEntityManager::CheckEntityCloseTimer(uint32_t entityId)
{
	uint64_t timerId = Timer()->setTimeout(10000, std::bind(&ServerEntityManager::EntityCloseTimer, this, placeholders::_1));

	AddTimerRecord(timerId, entityId);

	return timerId;
}

bool ServerEntityManager::RemoveEntity(uint32_t entityId)
{
	if (mEntityMap.contains(entityId))
	{
		ServerEntity* entity = &mEntityMap[entityId];
		unique_lock<shared_mutex> ulock(oMapMutex);

		mEntityMapList[entity->GetServerType()].remove(entity);

		if (ServerEntity* owner = entity->LinkNode())
		{
			owner->ClearFlag(ServerEntityFlag::Locked);
		}

		mEntityMap.erase(entityId);
		return true;
	}

	return false;
}
