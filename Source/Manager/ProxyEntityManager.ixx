module;
#include "StdMacro.h"
export module ProxyEntityManager;

import ProxyEntity;
import EntityManager;
import Logger;

export class ProxyEntityManager : public EntityManager<ProxyEntity>
{
	
public:
	ProxyEntityManager() = default;

	virtual ~ProxyEntityManager() = default;

	/// @brief 
	virtual bool Init() override
	{
		return EntityManager::Init();
	}

	/// @brief 
	virtual void TickMainFrame() override
	{
	}
	
public: // dll override

	/// @brief 
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
			DNPrint(0, EMLoggerLevel::Debug, "destory proxy Timer entity");
		}

	}

	/// @brief 
	uint64_t CheckEntityCloseTimer(uint32_t entityId)
	{
		uint64_t timerId = Timer()->setTimeout(10000, std::bind(&ProxyEntityManager::EntityCloseTimer, this, placeholders::_1));

		AddTimerRecord(timerId, entityId);

		return timerId;
	}

	/// @brief 
	bool RemoveEntity(uint32_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			unique_lock<shared_mutex> ulock(oMapMutex);

			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

protected: // dll proxy


};
