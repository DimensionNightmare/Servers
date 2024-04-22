module;
#include <map>
#include <shared_mutex>
#include "hv/Channel.h"

#include "StdAfx.h"
export module ServerEntityManagerHelper;

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
    ServerEntityHelper* AddEntity(uint32_t entityId, ServerType type);

    virtual bool RemoveEntity(uint32_t entityId);

	void MountEntity(ServerType type, TEntity* entity);

    void UnMountEntity(ServerType type, TEntity* entity);

    ServerEntityHelper* GetEntity(uint32_t id);

	list<TEntity*>& GetEntityByList(ServerType type);

	[[nodiscard]] uint32_t ServerIndex();
};

template <class TEntity>
ServerEntityHelper* ServerEntityManagerHelper<TEntity>::AddEntity(uint32_t entityId, ServerType regType)
{
	ServerEntityHelper* entity = nullptr;

	if (!this->mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(this->oMapMutex);

		TEntity* oriEntity = &this->mEntityMap[entityId];
		this->mEntityMapList[regType].emplace_back(oriEntity);
		
		entity = static_cast<ServerEntityHelper*>(oriEntity);
		entity->ID() = entityId;
		entity->ServerEntityType() = regType;
	}

	return entity;
}

template <class TEntity>
bool ServerEntityManagerHelper<TEntity>::RemoveEntity(uint32_t entityId)
{
	if(this->mEntityMap.contains(entityId))
	{
		TEntity* oriEntity = &this->mEntityMap[entityId];
		ServerEntityHelper* entity = static_cast<ServerEntityHelper*>(oriEntity);
		
		unique_lock<shared_mutex> ulock(this->oMapMutex);

		this->mEntityMapList[entity->ServerEntityType()].remove(oriEntity);
		
		DNPrint(0, LoggerLevel::Debug, "offline destory entity\n");
		this->mEntityMap.erase(entityId);
		return true;
	}
	
	return false;
}

template <class TEntity>
void ServerEntityManagerHelper<TEntity>::MountEntity(ServerType type, TEntity *entity)
{
	unique_lock<shared_mutex> ulock(this->oMapMutex);
	// mEntityMap mEntityMapList
	this->mEntityMapList[type].push_back(entity);
}

template <class TEntity>
void ServerEntityManagerHelper<TEntity>::UnMountEntity(ServerType type, TEntity *entity)
{
	unique_lock<shared_mutex> ulock(this->oMapMutex);
	// mEntityMap mEntityMapList
	this->mEntityMapList[type].remove(entity);
}

template <class TEntity>
ServerEntityHelper* ServerEntityManagerHelper<TEntity>::GetEntity(uint32_t entityId)
{
	shared_lock<shared_mutex> lock(this->oMapMutex);
	ServerEntityHelper* entity = nullptr;
	if(this->mEntityMap.contains(entityId))
	{
		TEntity* oriEntity = &this->mEntityMap[entityId];
		return static_cast<ServerEntityHelper*>(oriEntity);
	}
	// allow return empty
	return entity;
}

template <class TEntity>
list<TEntity*>& ServerEntityManagerHelper<TEntity>::GetEntityByList(ServerType type)
{
	shared_lock<shared_mutex> lock(this->oMapMutex);
	return this->mEntityMapList[type];
}

template <class TEntity>
uint32_t ServerEntityManagerHelper<TEntity>::ServerIndex()
{
	// if(this->mIdleServerId.size() > 0)
	// {
	// 	uint32_t index = this->mIdleServerId.front();
	// 	this->mIdleServerId.pop_front();
	// 	return index;
	// }
	return ++this->iServerId;
}
