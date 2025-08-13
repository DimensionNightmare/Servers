export module ProxyEntityManager;

import ProxyEntity;
import EntityManager;

export class ProxyEntityManager : public EntityManager<ProxyEntity>
{
protected:
	friend class System;
	friend class UniversalMemoryPool;
	/// @brief timer manager create
	ProxyEntityManager(System::WPtr system):EntityManager(system)
	{
		eComponentType = EMComponentType::ProxyEntityManager;

		pCheckEntityCloseTimer = std::bind(&ProxyEntityManager::CheckEntityCloseTimer, this, std::placeholders::_1);
		pAddEntity = std::bind(&ProxyEntityManager::AddEntity, this, std::placeholders::_1);
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
		if (RemoveEntity(entityId))
		{
			GetLogger()->Record(ELogLevel_Debug, "destory proxy Timer entity");
		}

	}

	/// @brief 
	uint64_t CheckEntityCloseTimer(uint64_t entityId)
	{
		uint64_t timerId = Timer()->setTimeout(10000, std::bind(&ProxyEntityManager::EntityCloseTimer, this, std::placeholders::_1));

		AddTimerRecord(timerId, entityId);

		return timerId;
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

	ProxyEntity::Ptr AddEntity(uint64_t entityId)
	{
		// ProxyEntity::CVPtr entity = std::shared_ptr<ProxyEntity>(new ProxyEntity(GetOwner()->GetWorldW()));
		ProxyEntity::Ptr entity = MemPool->Allocate<ProxyEntity, World::WPtr>(GetOwner()->GetWorldW());
		entity->SetID(entityId);

		std::unique_lock ulock(oMapMutex);
		mEntityMap[entityId] = entity;
		return entity;
	}

public:
	std::function<ProxyEntity::Ptr(uint64_t)> pAddEntity;

};
