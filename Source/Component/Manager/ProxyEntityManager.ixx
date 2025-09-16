export module ProxyEntityManager;

import ProxyEntity;
import EntityManager;
import FuncUtils;

export class ProxyEntityManager : public EntityManager<ProxyEntity>
{
protected:
	friend class System;
	friend class UniversalMemoryPool;
	/// @brief timer manager create
	ProxyEntityManager(System::WPtr system):EntityManager(system)
		,CheckEntityCloseTimer(this)
		,AddEntity(this)
	{
		eComponentType = EMComponentType::ProxyEntityManager;
	}
public:

	virtual ~ProxyEntityManager()
	{
		
	}

	virtual void Dispose() override
	{
		EntityManager::Dispose();
	}

	/// @brief 
	virtual void TickMainFrame() override
	{
	}
	
	/// @brief 
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
			GetLogger()->Record(ELogLevel_Debug, "destory proxy Timer entity");
		}

	}

public: // dll proxy

	/// @brief 
	bool RemoveEntity(uint64_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			std::unique_lock ulock(oMapMutex);
			ProxyEntity::CVPtr entity = mEntityMap[entityId];
			entity->Dispose();
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

	ProxyEntity::Ptr _AddEntity(uint64_t entityId)
	{
		ProxyEntity::Ptr entity = MemPool->Allocate<ProxyEntity, World::WPtr>(GetOwner()->GetWorldW());
		entity->SetID(entityId);

		std::unique_lock ulock(oMapMutex);
		mEntityMap[entityId] = entity;
		return entity;
	}
	
	/// @brief 
	uint64_t _CheckEntityCloseTimer(uint64_t entityId)
	{
		FunctionContainer<&ProxyEntityManager::EntityCloseTimer> funcProxy(this);

		uint64_t timerId = GetTimer()->SetTimeout(10000, funcProxy);

		AddTimerRecord(timerId, entityId);

		return timerId;
	}

public:
	
	FunctionContainer<&ProxyEntityManager::_AddEntity> AddEntity;
	FunctionContainer<&ProxyEntityManager::_CheckEntityCloseTimer> CheckEntityCloseTimer;

};
