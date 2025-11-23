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
	ServerEntityManager(System::CVPtr system):EntityManager(system)
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
		mEntityMapList.clear();
		
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
			// if(rm->GetChannel())
			{
				LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "EntityCloseTimer {} server destory entity, entity:{}, timer:{}", EnumName(entity->GetServerType()), entityId, timerID);
			}
		}
	}


protected:

	/// @brief 
	ServerEntity::Ptr RemoveEntity(size_t entityId)
	{
		ServerEntity::Ptr entity;

		if (mEntityMap.contains(entityId))
		{
			std::unique_lock ulock(oMapMutex);
			entity = std::move(mEntityMap[entityId]);

			// remove link

			auto RemoveLink = [](ServerEntity::CVPtr entity)
			{
				if (ServerEntity::CVPtr owner = entity->LinkNode())
				{
					owner->ClearFlag(EMServerEntityFlag::Locked);
					owner->GetMapLinkNode(entity->GetServerType()).remove(entity);

					entity->SetLinkNode(nullptr);
				}
			};

			// 不是gate
			RemoveLink(entity);

			//是gate
			auto allChild = entity->GetMapLink()
				| std::views::values
				| std::views::join
				| std::ranges::to<std::vector<ServerEntity::Ptr>>();

			for(const auto& child : allChild)
			{
				RemoveLink(child);
			}


			mEntityMapList[entity->GetServerType()].remove(entity);
			mEntityMap.erase(entityId);
		}

		return entity;
	}

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
		EventContainer<&ServerEntityManager::EntityCloseTimer> funcProxy(this);

		size_t timerId = GetTimer()->SetTimeout(10000, funcProxy);

		AddTimerRecord(timerId, entityId);

		return timerId;
	}

public:

	EventContainer<&ServerEntityManager::_AddEntity> AddEntity;
	EventContainer<&ServerEntityManager::_CheckEntityCloseTimer> CheckEntityCloseTimer;

protected: // dll proxy
	/// @brief 
	std::unordered_map<EMServerType, std::list<ServerEntity::Ptr> > mEntityMapList;
};
