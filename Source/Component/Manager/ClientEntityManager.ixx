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

		EntityManager::Dispose();

		mDbFailure.clear();
	}

public: // dll proxy

	bool RemoveEntity(size_t entityId)
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

	ClientEntity::Ptr _AddEntity(size_t entityId)
	{
		ClientEntity::Ptr entity = P_InstanceHolder->GetMemPool().Allocate<ClientEntity>(GetWorld());
		entity->SetID(entityId);

		std::unique_lock ulock(oMapMutex);
		mEntityMap[entityId] = entity;
		return entity;
	}

public:
	EventContainer<&ClientEntityManager::_AddEntity> AddEntity;

protected:

	/// @brief if save error. bin data will record to this.
	std::unordered_map<size_t, std::string> mDbFailure;

	size_t iSaveTimerId = 0;
	
};
