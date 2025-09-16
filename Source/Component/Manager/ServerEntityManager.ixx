export module ServerEntityManager;

import ServerEntity;
import EntityManager;
import Server;
import FuncUtils;

export class ServerEntityManager : public EntityManager<ServerEntity>
{
protected:
	friend class System;
	friend class UniversalMemoryPool;
	/// @brief timer manager create
	ServerEntityManager(System::WPtr system):EntityManager(system)
		,CheckEntityCloseTimer(this)
		,AddEntity(this)
	{
		eComponentType = EMComponentType::ServerEntityManager;

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

		mMapTimer.erase(timerID);

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

	ServerEntity::Ptr _AddEntity(uint64_t entityId, EMServerType regType)
	{
		ServerEntity::Ptr entity = MemPool->Allocate<ServerEntity, World::WPtr>(GetOwner()->GetWorldW());
		entity->SetID(entityId);

		std::unique_lock ulock(oMapMutex);
		mEntityMap[entityId] = entity;
		return entity;
	}

	/// @brief 
	uint64_t _CheckEntityCloseTimer(uint64_t entityId)
	{
		FunctionContainer<&ServerEntityManager::EntityCloseTimer> funcProxy(this);

		uint64_t timerId = GetTimer()->SetTimeout(10000, funcProxy);

		AddTimerRecord(timerId, entityId);

		return timerId;
	}

public:

	FunctionContainer<&ServerEntityManager::_AddEntity> AddEntity;
	FunctionContainer<&ServerEntityManager::_CheckEntityCloseTimer> CheckEntityCloseTimer;

protected: // dll proxy
	/// @brief 
	std::unordered_map<EMServerType, std::list<ServerEntity::Ptr> > mEntityMapList;
};
