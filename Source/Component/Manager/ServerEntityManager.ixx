export module ServerEntityManager;

import ServerEntity;
import EntityManager;
import Server;

export class ServerEntityManager : public EntityManager<ServerEntity>
{
protected:
	friend class System;
	friend class UniversalMemoryPool;
	/// @brief timer manager create
	ServerEntityManager(System::WPtr system):EntityManager(system)
	{
		eComponentType = EMComponentType::ServerEntityManager;

		pCheckEntityCloseTimer = std::bind(&ServerEntityManager::CheckEntityCloseTimer, this, std::placeholders::_1);
		pAddEntity = std::bind(&ServerEntityManager::AddEntity, this, std::placeholders::_1, std::placeholders::_2);
	}
public:

	virtual ~ServerEntityManager()
	{
	}

	virtual void Dispose() override
	{
		EntityManager::Dispose();

		mEntityMapList.clear();
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

		if(mEntityMap.count(entityId))
		{
			ServerEntity::CVPtr rm = mEntityMap[entityId];
			if(ServerEntity::CVPtr link = rm->LinkNode())
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
			ServerEntity::CVPtr entity = mEntityMap[entityId];

			if (ServerEntity::CVPtr owner = entity->LinkNode())
			{
				owner->ClearFlag(EMServerEntityFlag::Locked);
			}

			entity->Dispose();

			std::unique_lock ulock(oMapMutex);
			mEntityMapList[entity->GetServerType()].remove(entity);
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

	ServerEntity::Ptr AddEntity(uint64_t entityId, EMServerType regType)
	{
		// ServerEntity::CVPtr entity = std::shared_ptr<ServerEntity>(new ServerEntity(GetOwner()->GetWorldW()));
		ServerEntity::Ptr entity = MemPool->Allocate<ServerEntity, World::WPtr>(GetOwner()->GetWorldW());
		entity->SetID(entityId);

		std::unique_lock ulock(oMapMutex);
		mEntityMap[entityId] = entity;
		mEntityMapList[regType].emplace_back(entity);
		return entity;
	}

public:

	std::function<ServerEntity::Ptr(uint64_t,EMServerType)> pAddEntity;

protected: // dll proxy
	/// @brief 
	std::unordered_map<EMServerType, std::list<ServerEntity::Ptr> > mEntityMapList;
};
