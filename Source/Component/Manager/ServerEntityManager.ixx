module;
export module ServerEntityManager;

import ServerEntity;
import EntityManager;
import DNServer;

export class ServerEntityManager : public EntityManager<ServerEntity>
{
protected:
	friend class System;
	/// @brief timer manager create
	ServerEntityManager(System::WPtr system):EntityManager(system)
	{
		eComponentType = EMComponentType::ServerEntityManager;
	}
public:

	virtual ~ServerEntityManager() = default;

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

		if(mEntityMap.count(entityId))
		{
			ServerEntity::Ptr rm = mEntityMap[entityId];
			if(const ServerEntity::Ptr& link = rm->LinkNode())
			{
				link->GetMapLinkNode(rm->GetServerType()).remove(rm);
			}

			RemoveEntity(entityId);
			
			GetLogger()->Record(ELogLevel_Debug, "EntityCloseTimer server destory entity");
			
		}
	}

	/// @brief 
	uint64_t CheckEntityCloseTimer(uint64_t entityId)
	{
		uint64_t timerId = Timer()->setTimeout(10000, std::bind(&ServerEntityManager::EntityCloseTimer, this, std::placeholders::_1));

		AddTimerRecord(timerId, entityId);

		return timerId;
	}

public: // dll override

	/// @brief 
	bool RemoveEntity(uint64_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			ServerEntity::Ptr entity = mEntityMap[entityId];

			if (const ServerEntity::Ptr& owner = entity->LinkNode())
			{
				owner->ClearFlag(EMServerEntityFlag::Locked);
			}

			entity->Dispose();

			std::unique_lock<std::shared_mutex> ulock(oMapMutex);
			mEntityMapList[entity->GetServerType()].remove(entity);
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

	void AddEntity(uint64_t entityId, EMServerType regType)
	{
		ServerEntity::Ptr entity = std::shared_ptr<ServerEntity>(new ServerEntity(GetOwner()->GetWorldW()));
		entity->SetID(entityId);

		std::unique_lock<std::shared_mutex> ulock(oMapMutex);
		mEntityMap[entityId] = entity;
		mEntityMapList[regType].emplace_back(entity);
	}

protected: // dll proxy
	/// @brief 
	std::unordered_map<EMServerType, std::list<ServerEntity::Ptr> > mEntityMapList;
};
