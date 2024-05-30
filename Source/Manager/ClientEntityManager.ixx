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

	virtual ~ClientEntityManager() = default;

	virtual bool Init() override;

public: // dll override
	bool RemoveEntity(uint32_t entityId);

protected: // dll proxy


};

bool ClientEntityManager::Init()
{
	return EntityManager::Init();
}

bool ClientEntityManager::RemoveEntity(uint32_t entityId)
{
	if (mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(oMapMutex);

		mEntityMap.erase(entityId);
		return true;
	}

	return false;
}