export module ClientEntityManager;

import ClientEntity;
import EntityManager;
import ClientProxy;
import StrUtils;
import MdbProxy;

/// @brief manager client proxys
export class ClientEntityManager : public EntityManager<ClientEntity>
{
	
protected:
	friend class System;
	friend class UniversalMemoryPool;
	/// @brief timer manager create
	ClientEntityManager(System::WPtr system):EntityManager(system)
	{
		eComponentType = EMComponentType::ClientEntityManager;

		pAddEntity = std::bind(&ClientEntityManager::AddEntity, this, std::placeholders::_1);
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

	ClientEntity::Ptr AddEntity(uint64_t entityId)
	{
		// ClientEntity::CVPtr entity = std::shared_ptr<ClientEntity>(new ClientEntity(GetOwner()->GetWorldW()));
		ClientEntity::Ptr entity = MemPool->Allocate<ClientEntity, World::WPtr>(GetOwner()->GetWorldW());;
		entity->SetID(entityId);
		entity->GetDbEntity()->set_account_id(entityId);

		std::unique_lock ulock(oMapMutex);
		mEntityMap[entityId] = entity;
		return entity;
	}

	bool RemoveEntity(uint64_t entityId)
	{

		if (mEntityMap.contains(entityId))
		{
			GetLogger()->Record(ELogLevel_Debug, "destory client entity");
			ClientEntity::Ptr& entity = mEntityMap[entityId];
			entity->Dispose();

			std::unique_lock ulock(oMapMutex);
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

public:
	std::function<ClientEntity::Ptr(uint64_t)> pAddEntity;

protected: // dll proxy
	ClientProxy::Ptr pSqlClient;

	/// @brief if save error. bin data will record to this.
	std::unordered_map<uint64_t, std::string> mDbFailure;
	
};
