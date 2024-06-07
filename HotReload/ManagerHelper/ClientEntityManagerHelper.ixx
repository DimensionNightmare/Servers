module;
#include <shared_mutex>
#include <cstdint>
#include <coroutine>

#include "StdMacro.h"
export module ClientEntityManagerHelper;

export import ClientEntityHelper;
export import ClientEntityManager;
import Logger;
import DNTask;

using namespace std;

export class ClientEntityManagerHelper : public ClientEntityManager
{
private:
	ClientEntityManagerHelper() = delete;
public:
	ClientEntity* AddEntity(uint32_t entityId);

	bool RemoveEntity(uint32_t entityId);

	ClientEntity* GetEntity(uint32_t id);

	DNTaskVoid LoadEntityData();
};

ClientEntity* ClientEntityManagerHelper::AddEntity(uint32_t entityId)
{
	if (!mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(oMapMutex);
		ClientEntity* entity = &mEntityMap[entityId];
		return entity;
	}

	return nullptr;
}

bool ClientEntityManagerHelper::RemoveEntity(uint32_t entityId)
{

	if (mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(oMapMutex);

		DNPrint(0, LoggerLevel::Debug, "destory client entity");
		mEntityMap.erase(entityId);

		return true;
	}

	return false;
}


ClientEntity* ClientEntityManagerHelper::GetEntity(uint32_t entityId)
{
	shared_lock<shared_mutex> lock(oMapMutex);
	if (mEntityMap.contains(entityId))
	{
		return &mEntityMap[entityId];
	}
	// allow return empty
	return nullptr;
}

DNTaskVoid ClientEntityManagerHelper::LoadEntityData()
{
	co_return;
}
