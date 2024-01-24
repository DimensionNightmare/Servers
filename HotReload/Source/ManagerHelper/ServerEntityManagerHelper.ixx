module;
#include "hv/Channel.h"

#include <map>
#include <shared_mutex>
export module ServerEntityManagerHelper;

import ServerEntity;
import ServerEntityHelper;
import ServerEntityManager;

using namespace std;
using namespace hv;

export template<class TEntity = ServerEntity>
class ServerEntityManagerHelper : public ServerEntityManager<TEntity>
{
private:
	ServerEntityManagerHelper(){}
public:
    auto AddEntity(unsigned int entityId, ServerType type);

    void RemoveEntity(unsigned int entityId, bool isDel = true);

	void MountEntity(ServerType type, TEntity* entity);

    void UnMountEntity(ServerType type, TEntity* entity);

    auto GetEntity(unsigned int id);

	auto& GetEntityByList(ServerType type);

	[[nodiscard]] unsigned int GetServerIndex();
};

template <class TEntity>
auto ServerEntityManagerHelper<TEntity>::AddEntity(unsigned int entityId, ServerType regType)
{
	unique_lock<shared_mutex> ulock(this->oMapMutex);

	ServerEntityHelper* entity = nullptr;

	if (!this->mEntityMap.count(entityId))
	{
		auto oriEntity = new TEntity;
		this->mEntityMap.emplace(entityId, oriEntity);
		this->mEntityMapList[regType].emplace_back(oriEntity);
		
		entity = static_cast<ServerEntityHelper*>(oriEntity);
		entity->GetChild()->SetID(entityId);
		entity->SetServerType(regType);
	}

	return entity;
}

template <class TEntity>
void ServerEntityManagerHelper<TEntity>::RemoveEntity(unsigned int entityId, bool isDel)
{
	unique_lock<shared_mutex> ulock(this->oMapMutex);
	if(this->mEntityMap.count(entityId))
	{
		auto oriEntity = this->mEntityMap[entityId];
		auto entity = static_cast<ServerEntityHelper*>(oriEntity);
		if(isDel)
		{
			printf("destory entity\n");
			this->mEntityMap.erase(entityId);
			this->mIdleServerId.push_back(entityId);
			this->mEntityMapList[entity->GetServerType()].remove(oriEntity);
			delete oriEntity;
		}
		else
		{
			entity->GetChild()->SetSock(nullptr);
		}
	}
	
}

template <class TEntity>
void ServerEntityManagerHelper<TEntity>::UnMountEntity(ServerType type, TEntity *entity)
{
	unique_lock<shared_mutex> ulock(this->oMapMutex);
	// mEntityMap mEntityMapList
	this->mEntityMapList[type].remove(entity);
}

template <class TEntity>
auto ServerEntityManagerHelper<TEntity>::GetEntity(unsigned int entityId)
{
	shared_lock<shared_mutex> lock(this->oMapMutex);
	ServerEntityHelper* entity = nullptr;
	if(this->mEntityMap.count(entityId))
	{
		auto oriEntity = this->mEntityMap[entityId];
		return static_cast<ServerEntityHelper*>(oriEntity);
	}
	// allow return empty
	return entity;
}

template <class TEntity>
auto& ServerEntityManagerHelper<TEntity>::GetEntityByList(ServerType type)
{
	shared_lock<shared_mutex> lock(this->oMapMutex);
	return this->mEntityMapList[type];
}

template <class TEntity>
unsigned int ServerEntityManagerHelper<TEntity>::GetServerIndex()
{
	if(this->mIdleServerId.size() > 0)
	{
		auto index = this->mIdleServerId.front();
		this->mIdleServerId.pop_front();
		return index;
	}
	return this->iServerId++;
}
