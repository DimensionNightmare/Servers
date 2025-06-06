module;
export module ProxyEntityManager;

import ProxyEntity;
import EntityManager;

export class ProxyEntityManager : public EntityManager<ProxyEntity>
{
protected:
	friend class System;
	/// @brief timer manager create
	ProxyEntityManager(System::WPtr system):EntityManager(system)
	{
		eComponentType = EMComponentType::ProxyEntityManager;
	}
public:

	virtual ~ProxyEntityManager() = default;

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
		std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
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
			std::unique_lock<std::shared_mutex> ulock(oMapMutex);
			ProxyEntity::Ptr entity = mEntityMap[entityId];
			entity->Dispose();
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

	void AddEntity(uint64_t entityId)
	{
		ProxyEntity::Ptr entity = std::shared_ptr<ProxyEntity>(new ProxyEntity(GetOwner()->GetWorldW()));
		entity->SetID(entityId);

		std::unique_lock<std::shared_mutex> ulock(oMapMutex);
		mEntityMap[entityId] = entity;
	}

};
