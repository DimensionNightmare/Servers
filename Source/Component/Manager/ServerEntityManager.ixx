module;
export module ServerEntityManager;

import ServerEntity;
import EntityManager;
import Logger;
import DNServer;

export class ServerEntityManager : public EntityManager<ServerEntity>
{
protected:
	friend class System;
	/// @brief timer manager create
	ServerEntityManager(System::Ptr system):EntityManager(system)
	{
		eComponentType = EMComponentType::ServerEntityManager;
	}
public:

	virtual ~ServerEntityManager() = default;

	/// @brief 
	virtual void TickMainFrame() override
	{
	}

public: // dll override

	/// @brief 
	void EntityCloseTimer(uint64_t timerID)
	{
		std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
		if (!mMapTimer.contains(timerID))
		{
			return;
		}

		uint32_t entityId = mMapTimer[timerID];

		if(ServerEntity* rm = GetEntity(entityId))
		{
			if(ServerEntity* link = rm->LinkNode())
			{
				link->GetMapLinkNode(rm->GetServerType()).remove(rm);
			}

			RemoveEntity(entityId);
			
			SPidLogger.Record(ELogLevel_Debug, "EntityCloseTimer server destory entity");
			
		}
	}

	/// @brief 
	uint64_t CheckEntityCloseTimer(uint32_t entityId)
	{
		uint64_t timerId = Timer()->setTimeout(10000, std::bind(&ServerEntityManager::EntityCloseTimer, this, std::placeholders::_1));

		AddTimerRecord(timerId, entityId);

		return timerId;
	}

	/// @brief 
	ServerEntity* GetEntity(uint32_t entityId)
	{
		std::unique_lock<std::shared_mutex> ulock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return &mEntityMap[entityId];
		}
		return nullptr;
	}

	/// @brief 
	bool RemoveEntity(uint32_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			ServerEntity::Ptr entity = mEntityMap[entityId];
			std::unique_lock<std::shared_mutex> ulock(oMapMutex);

			mEntityMapList[entity->GetServerType()].remove(entity);

			if (ServerEntity* owner = entity->LinkNode())
			{
				owner->ClearFlag(EMServerEntityFlag::Locked);
			}

			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

protected: // dll proxy
	/// @brief 
	std::unordered_map<EMServerType, std::list<ServerEntity*> > mEntityMapList;
	
	// server pull server
	std::atomic<uint32_t> iServerGenId;

};
