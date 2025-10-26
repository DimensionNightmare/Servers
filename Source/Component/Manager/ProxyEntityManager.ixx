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
			LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "destory proxy Timer entity");
		}

	}

public: // dll proxy

	/// @brief 
	bool RemoveEntity(size_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			std::unique_lock ulock(oMapMutex);
			ProxyEntity::Ptr entity = mEntityMap[entityId];
			entity->Dispose();
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

protected:

	ProxyEntity::Ptr _AddEntity(size_t entityId)
	{
		ProxyEntity::Ptr entity = P_InstanceHolder->GetMemPool().Allocate<ProxyEntity>(GetOwner()->GetWorldW());
		entity->SetID(entityId);

		std::unique_lock ulock(oMapMutex);
		mEntityMap[entityId] = entity;
		return entity;
	}
	
	/// @brief 
	size_t _CheckEntityCloseTimer(size_t entityId)
	{
		FunctionContainer<&ProxyEntityManager::EntityCloseTimer> funcProxy(this);

		size_t timerId = GetTimer()->SetTimeout(10000, funcProxy);

		AddTimerRecord(timerId, entityId);

		return timerId;
	}

public:
	
	FunctionContainer<&ProxyEntityManager::_AddEntity> AddEntity;
	FunctionContainer<&ProxyEntityManager::_CheckEntityCloseTimer> CheckEntityCloseTimer;

};
