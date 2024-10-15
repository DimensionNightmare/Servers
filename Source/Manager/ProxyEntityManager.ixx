module;
#include "StdMacro.h"
export module ProxyEntityManager;

export import ProxyEntity;
import EntityManager;
import Logger;

export class ProxyEntityManager : public EntityManager<ProxyEntity>
{
public:
	ProxyEntityManager() = default;

	virtual ~ProxyEntityManager() = default;

	virtual bool Init() override;

	virtual void TickMainFrame() override;

public: // dll override
	void EntityCloseTimer(uint64_t timerID);
	uint64_t CheckEntityCloseTimer(uint32_t entityId);

	bool RemoveEntity(uint32_t entityId);

protected: // dll proxy


};

extern "C"
{
	REGIST_MAINSPACE_SIGN_FUNCTION(ProxyEntityManager, CheckEntityCloseTimer);
}

bool ProxyEntityManager::Init()
{
	return EntityManager::Init();
}

void ProxyEntityManager::TickMainFrame()
{
}

void ProxyEntityManager::EntityCloseTimer(uint64_t timerID)
{
	unique_lock<shared_mutex> ulock(oTimerMutex);
	if (!mMapTimer.contains(timerID))
	{
		return;
	}

	uint32_t entityId = mMapTimer[timerID];
	if (RemoveEntity(entityId))
	{
		DNPrint(0, LoggerLevel::Debug, "destory proxy Timer entity");
	}

}

uint64_t ProxyEntityManager::CheckEntityCloseTimer(uint32_t entityId)
{
	uint64_t timerId = Timer()->setTimeout(10000, std::bind(&ProxyEntityManager::EntityCloseTimer, this, placeholders::_1));

	AddTimerRecord(timerId, entityId);

	return timerId;
}

bool ProxyEntityManager::RemoveEntity(uint32_t entityId)
{
	if (mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(oMapMutex);

		mEntityMap.erase(entityId);
		return true;
	}

	return false;
}