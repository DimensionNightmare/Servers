module;
#include <unordered_map>
#include <list>
#include <shared_mutex>
#include <cstdint>

#include "StdMacro.h"
export module ClientEntityManager;

export import ClientEntity;
import EntityManager;

using namespace std;

export class ClientEntityManager : public EntityManager<ClientEntity>
{
public:
	ClientEntityManager() = default;

	virtual ~ClientEntityManager();

	virtual bool Init() override;
	
	virtual void TickMainFrame() override;

public: // dll override

	bool SaveEntity(ClientEntity& entity);

	void CheckSaveEntity();

protected: // dll proxy


};

ClientEntityManager::~ClientEntityManager()
{
	CheckSaveEntity();
}

bool ClientEntityManager::Init()
{
	return EntityManager::Init();
}

bool ClientEntityManager::SaveEntity(ClientEntity& entity)
{
	// nosql

	// sql

	return false;
}

void ClientEntityManager::CheckSaveEntity()
{
	for (auto& [ID, entity] : mEntityMap)
	{
		if(entity.HasFlag(ClientEntityFlag::DBModify))
		{
			entity.ClearFlag(ClientEntityFlag::DBModify);
			
			SaveEntity(entity);
		}
	}
}

void ClientEntityManager::TickMainFrame()
{
	CheckSaveEntity();
}
