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
	template<class CastTEntity = ServerEntityHelper>
    CastTEntity* AddEntity(const SocketChannelPtr& channel, unsigned int entityId, ServerType serverType);

	template<class CastTEntity = ServerEntityHelper>
    void RemoveEntity(const SocketChannelPtr& channel, bool isDel = true);

	template<class CastTEntity = ServerEntityHelper>
    CastTEntity* GetEntity(unsigned int id);

	auto& GetEntity(ServerType type);

	[[nodiscard]] unsigned int GetServerIndex();
};

template <class TEntity>
template <class CastTEntity>
CastTEntity* ServerEntityManagerHelper<TEntity>::AddEntity(const SocketChannelPtr& channel, unsigned int entityId, ServerType serverType)
{
	unique_lock<shared_mutex> ulock(this->oMapMutex);

	TEntity* entity = nullptr;

	CastTEntity* castEntity = nullptr;

	if (this->mEntityMap.count(entityId))
	{
		entity = this->mEntityMap[entityId];
		castEntity = static_cast<CastTEntity*>(entity);
	}
	else
	{
		entity = new TEntity;
		this->mEntityMap.emplace(entityId, entity);
		channel->setContext(entity);

		castEntity = static_cast<CastTEntity*>(entity);
		castEntity->GetChild()->SetID(entityId);

		castEntity->SetServerType(serverType);

		this->mEntityMapList[serverType].emplace_back(entity);
	}

	castEntity->GetChild()->SetSock(channel);

	return castEntity;
}

template <class TEntity>
template <class CastTEntity>
void ServerEntityManagerHelper<TEntity>::RemoveEntity(const SocketChannelPtr& channel, bool isDel)
{
	unique_lock<shared_mutex> ulock(this->oMapMutex);
	if(TEntity* entity = channel->getContext<TEntity>())
	{
		printf("destory entity\n");
		channel->setContext(nullptr);
		auto castEntity = static_cast<CastTEntity*>(entity);
		if(isDel)
		{
			auto serverIndex = castEntity->GetChild()->GetID();
			this->mEntityMap.erase(serverIndex);
			this->mIdleServerId.push_back(serverIndex);
			this->mEntityMapList[castEntity->GetServerType()].remove(entity);
			delete entity;
		}
		else
		{
			castEntity->GetChild()->SetSock(nullptr);
		}
	}
	
}

template <class TEntity>
template <class CastTEntity>
CastTEntity *ServerEntityManagerHelper<TEntity>::GetEntity(unsigned int id)
{
	shared_lock<shared_mutex> lock(this->oMapMutex);
	// allow return empty
	return static_cast<CastTEntity*>(this->mEntityMap[id]);
}

template <class TEntity>
auto& ServerEntityManagerHelper<TEntity>::GetEntity(ServerType type)
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
