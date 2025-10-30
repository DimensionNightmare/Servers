export module ServerEntityManager;

import ServerEntity;
import EntityManager;
import Server;
import FuncUtils;
import Logger;
import StrUtils;

export class ServerEntityManager : public EntityManager<ServerEntity>
{
protected:
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
	void EntityCloseTimer(size_t timerID)
	{
		std::unique_lock ulock(oTimerMutex);
		if (!mMapTimer.contains(timerID))
		{
			return;
		}

		size_t entityId = mMapTimer[timerID];

		mMapTimer.erase(timerID);

		if(mEntityMap.count(entityId))
		{
			ServerEntity::Ptr rm = mEntityMap[entityId];
			if(ServerEntity::Ptr link = rm->LinkNode())
			{
				link->GetMapLinkNode(rm->GetServerType()).remove(rm);
			}

			RemoveEntity(entityId);
			
			// if(rm->GetChannel())
			{
				LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "EntityCloseTimer {} server destory entity, entity:{}, timer:{}", EnumName(rm->GetServerType()), entityId, timerID);
			}
			
		}
	}


public: // dll override

	/// @brief 
	bool RemoveEntity(size_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			ServerEntity::Ptr entity = mEntityMap[entityId];

			if (ServerEntity::Ptr owner = entity->LinkNode())
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

protected:

	ServerEntity::Ptr _AddEntity(size_t entityId, EMServerType regType)
	{
		ServerEntity::Ptr entity = P_InstanceHolder->GetMemPool().Allocate<ServerEntity>(GetWorld());
		entity->SetID(entityId);

		std::unique_lock ulock(oMapMutex);
		mEntityMap[entityId] = entity;
		return entity;
	}

	/// @brief 
	size_t _CheckEntityCloseTimer(size_t entityId)
	{
		FunctionContainer<&ServerEntityManager::EntityCloseTimer> funcProxy(this);

		size_t timerId = GetTimer()->SetTimeout(10000, funcProxy);

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
