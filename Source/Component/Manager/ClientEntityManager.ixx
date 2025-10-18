export module ClientEntityManager;

import ClientEntity;
import EntityManager;
import ClientProxy;
import StrUtils;
import MdbProxy;
import FuncUtils;
import Logger;

/// @brief manager client proxys
export class ClientEntityManager : public EntityManager<ClientEntity>
{
	
protected:

	friend class UniversalMemoryPool;
	/// @brief timer manager create
	ClientEntityManager(System::WPtr system):EntityManager(system)
		,AddEntity(this)
	{
		eComponentType = EMComponentType::ClientEntityManager;
	}
public:

	virtual ~ClientEntityManager()
	{
		
	}

	virtual void Dispose() override
	{
		// CheckSaveEntity(true);

		EntityManager::Dispose();

		mDbFailure.clear();
	}

	virtual void TickMainFrame() override
	{
		// CheckSaveEntity();
	}

public: // dll proxy

	bool RemoveEntity(uint64_t entityId)
	{

		if (mEntityMap.contains(entityId))
		{
			LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "destory client entity");
			ClientEntity::Ptr& entity = mEntityMap[entityId];
			entity->Dispose();

			std::unique_lock ulock(oMapMutex);
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

protected:

	ClientEntity::Ptr _AddEntity(uint64_t entityId)
	{
		ClientEntity::Ptr entity = P_InstanceHolder->MemPool->Allocate<ClientEntity, World::WPtr>(GetOwner()->GetWorldW());;
		entity->SetID(entityId);

		std::unique_lock ulock(oMapMutex);
		mEntityMap[entityId] = entity;
		return entity;
	}

public:
	FunctionContainer<&ClientEntityManager::_AddEntity> AddEntity;

protected: // dll proxy
	ClientProxy::Ptr pSqlClient;

	/// @brief if save error. bin data will record to this.
	std::unordered_map<uint64_t, std::string> mDbFailure;
	
};
