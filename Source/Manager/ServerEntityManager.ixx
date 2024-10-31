module;
#include "StdMacro.h"
export module ServerEntityManager;

import ServerEntity;
import EntityManager;
import Logger;
import DNServer;

export class ServerEntityManager : public EntityManager<ServerEntity>
{
	
public:
	ServerEntityManager() = default;

	virtual ~ServerEntityManager() = default;

	/// @brief 
	virtual bool Init() override
	{
		return EntityManager::Init();
	}

	/// @brief 
	virtual void TickMainFrame() override
	{
	}

public: // dll override

	/// @brief 
	void EntityCloseTimer(uint64_t timerID)
	{
		unique_lock<shared_mutex> ulock(oTimerMutex);
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
			
			DNPrint(0, EMLoggerLevel::Debug, "EntityCloseTimer server destory entity");
			
		}
	}

	/// @brief 
	uint64_t CheckEntityCloseTimer(uint32_t entityId)
	{
		uint64_t timerId = Timer()->setTimeout(10000, std::bind(&ServerEntityManager::EntityCloseTimer, this, placeholders::_1));

		AddTimerRecord(timerId, entityId);

		return timerId;
	}

	/// @brief 
	ServerEntity* GetEntity(uint32_t entityId)
	{
		unique_lock<shared_mutex> ulock(oMapMutex);
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
			ServerEntity* entity = &mEntityMap[entityId];
			unique_lock<shared_mutex> ulock(oMapMutex);

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
	unordered_map<EMServerType, list<ServerEntity*> > mEntityMapList;
	
	// server pull server
	atomic<uint32_t> iServerGenId;

};
