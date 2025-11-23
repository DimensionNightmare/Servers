export module ProxyEntityManager;

import ProxyEntity;
import EntityManager;
import FuncUtils;
import Logger;

export class ProxyEntityManager : public EntityManager<ProxyEntity>
{
protected:

	friend class UniversalMemoryPool;
	/// @brief timer manager create
	ProxyEntityManager(System::CVPtr system):EntityManager(system)
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
	void EntityCloseTimer(size_t timerID)
	{
		std::unique_lock ulock(oTimerMutex);
		if (!mMapTimer.contains(timerID))
		{
			return;
		}

		size_t entityId = mMapTimer[timerID];

		mMapTimer.erase(timerID);

		if (auto entity = RemoveEntity(entityId))
		{
			entity->Dispose();
			LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "destory proxy Timer entity {}", entity->ID());
		}

	}

protected:

	/// @brief 
	ProxyEntity::Ptr RemoveEntity(size_t entityId)
	{
		ProxyEntity::Ptr entity;

		if (mEntityMap.contains(entityId))
		{
			std::unique_lock ulock(oMapMutex);
			entity = std::move(mEntityMap[entityId]);
			mEntityMap.erase(entityId);
		}

		return entity;
	}

	ProxyEntity::Ptr _AddEntity(size_t entityId)
	{
		ProxyEntity::Ptr entity = P_InstanceHolder->GetMemPool().Allocate<ProxyEntity>(GetWorld());
		entity->SetID(entityId);

		std::unique_lock ulock(oMapMutex);
		mEntityMap[entityId] = entity;
		return entity;
	}
	
	/// @brief 
	size_t _CheckEntityCloseTimer(size_t entityId)
	{
		EventContainer<&ProxyEntityManager::EntityCloseTimer> funcProxy(this);

		size_t timerId = GetTimer()->SetTimeout(10000, funcProxy);

		AddTimerRecord(timerId, entityId);

		return timerId;
	}

public:
	
	EventContainer<&ProxyEntityManager::_AddEntity> AddEntity;
	EventContainer<&ProxyEntityManager::_CheckEntityCloseTimer> CheckEntityCloseTimer;

};
