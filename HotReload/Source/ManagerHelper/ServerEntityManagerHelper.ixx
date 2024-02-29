module;
#include "StdAfx.h"
#include "hv/Channel.h"

#include <map>
#include <shared_mutex>
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
    ServerEntityHelper* AddEntity(unsigned int entityId, ServerType type);

    void RemoveEntity(unsigned int entityId, bool isDel = true);

	void MountEntity(ServerType type, TEntity* entity);

    void UnMountEntity(ServerType type, TEntity* entity);

    ServerEntityHelper* GetEntity(unsigned int id);

	list<TEntity*>& GetEntityByList(ServerType type);

	[[nodiscard]] unsigned int GetServerIndex();
};

template <class TEntity>
ServerEntityHelper* ServerEntityManagerHelper<TEntity>::AddEntity(unsigned int entityId, ServerType regType)
{
	ServerEntityHelper* entity = nullptr;

	if (!this->mEntityMap.count(entityId))
	{
		unique_lock<shared_mutex> ulock(this->oMapMutex);

		TEntity* oriEntity = new TEntity;
		this->mEntityMap.emplace(entityId, oriEntity);
		this->mEntityMapList[regType].emplace_back(oriEntity);
		
		entity = static_cast<ServerEntityHelper*>(oriEntity);
		entity->GetChild()->ID() = entityId;
		entity->ServerEntityType() = regType;
	}

	return entity;
}

template <class TEntity>
void ServerEntityManagerHelper<TEntity>::RemoveEntity(unsigned int entityId, bool isDel)
{
	
	if(this->mEntityMap.count(entityId))
	{
		TEntity* oriEntity = this->mEntityMap[entityId];
		ServerEntityHelper* entity = static_cast<ServerEntityHelper*>(oriEntity);
		if(isDel)
		{
			unique_lock<shared_mutex> ulock(this->oMapMutex);

			DNPrint(-1, LoggerLevel::Debug, "destory entity\n");
			this->mEntityMap.erase(entityId);
			// this->mIdleServerId.push_back(entityId);
			this->mEntityMapList[entity->ServerEntityType()].remove(oriEntity);
			delete oriEntity;
		}
		else
		{
			entity->GetChild()->SetSock(nullptr);
		}
	}
	
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
ServerEntityHelper* ServerEntityManagerHelper<TEntity>::GetEntity(unsigned int entityId)
{
	shared_lock<shared_mutex> lock(this->oMapMutex);
	ServerEntityHelper* entity = nullptr;
	if(this->mEntityMap.count(entityId))
	{
		TEntity* oriEntity = this->mEntityMap[entityId];
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
unsigned int ServerEntityManagerHelper<TEntity>::GetServerIndex()
{
	// if(this->mIdleServerId.size() > 0)
	// {
	// 	unsigned int index = this->mIdleServerId.front();
	// 	this->mIdleServerId.pop_front();
	// 	return index;
	// }
	return ++this->iServerId;
}
