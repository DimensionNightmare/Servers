module;
#include "hv/Channel.h"
#include <map>
export module EntityManager;

export import Entity;

using namespace std;
using namespace hv;

export template<class TEntity = Entity>
class EntityManager
{
public:
    EntityManager();

	~EntityManager();

    decltype(auto) AddEntity(SocketChannelPtr channel, int entityId);

    void RemoveEntity(SocketChannelPtr channel);

protected:
    map<int, TEntity*> mEntityMap;
};

module:private;

template <class TEntity>
EntityManager<TEntity>::EntityManager()
{
	mEntityMap.clear();
}

template <class TEntity>
EntityManager<TEntity>::~EntityManager()
{
	for(auto& iter : mEntityMap)
	{
		delete iter.second;
		mEntityMap.erase(iter.first);
	}
}

template <class TEntity>
decltype(auto) EntityManager<TEntity>::AddEntity(SocketChannelPtr channel, int entityId)
{
	TEntity* entity = nullptr;
	if(mEntityMap.count(entityId))
	{
		entity = mEntityMap[entityId];
	}
	else
	{
		entity = new TEntity;
		entity->iId = entityId;
		mEntityMap.emplace(entityId, entity);
		channel->setContext(entity);
	}

	entity->pSock = channel;
	
	return entity;
}

template <class TEntity>
void EntityManager<TEntity>::RemoveEntity(SocketChannelPtr channel)
{
	if(TEntity* entity = channel->getContext<TEntity>())
	{
		channel->setContext(nullptr);
		mEntityMap.erase(entity->iId);
		delete entity;
	}
	
}