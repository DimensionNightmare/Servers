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
	ClientEntityManager(System::CVPtr system):EntityManager(system)
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
		mDbFailure.clear();

		EntityManager::Dispose();
	}


protected:

	ClientEntity::Ptr RemoveEntity(size_t entityId)
	{
		ClientEntity::Ptr entity;

		if (mEntityMap.contains(entityId))
		{
			LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "destory client entity");

			std::unique_lock ulock(oMapMutex);
			entity = std::move(mEntityMap[entityId]);
			mEntityMap.erase(entityId);
		}

		return entity;
	}

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
