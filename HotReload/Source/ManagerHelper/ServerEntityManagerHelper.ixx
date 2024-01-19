module;
#include "hv/Channel.h"

#include <map>
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
    void RemoveEntity(const SocketChannelPtr& channel);

	template<class CastTEntity = ServerEntityHelper>
    CastTEntity* GetEntity(unsigned int id);

	auto& GetEntity(ServerType type);

	[[nodiscard]] unsigned int GetServerIndex();
};

template <class TEntity>
template <class CastTEntity>
CastTEntity* ServerEntityManagerHelper<TEntity>::AddEntity(const SocketChannelPtr& channel, unsigned int entityId, ServerType serverType)
{
	TEntity* entity = nullptr;
	if (this->mEntityMap.count(entityId))
	{
		entity = this->mEntityMap[entityId];
	}
	else
	{
		entity = new TEntity;
		this->mEntityMap.emplace(entityId, entity);
		channel->setContext(entity);

		auto castEntity = static_cast<CastTEntity*>(entity);
		auto base = castEntity->GetChild();
		base->SetID(entityId);
		base->SetSock(channel);

		castEntity->SetServerType(serverType);

		this->mEntityMapList[serverType].emplace_back(entity);

		return castEntity;
	}

	

	return static_cast<CastTEntity*>(entity);
}

template <class TEntity>
template <class CastTEntity>
void ServerEntityManagerHelper<TEntity>::RemoveEntity(const SocketChannelPtr& channel)
{
	if(TEntity* entity = channel->getContext<TEntity>())
	{
		printf("destory entity\n");
		channel->setContext(nullptr);
		auto castEntity = static_cast<CastTEntity*>(entity);
		auto serverIndex = castEntity->GetChild()->GetID();
		this->mEntityMap.erase(serverIndex);
		this->mIdleServerId.push_back(serverIndex);
		this->mEntityMapList[castEntity->GetServerType()].remove(entity);
		delete entity;

	}
	
}

template <class TEntity>
template <class CastTEntity>
CastTEntity *ServerEntityManagerHelper<TEntity>::GetEntity(unsigned int id)
{
	// allow return empty
	return static_cast<CastTEntity*>(this->mEntityMap[id]);
}

template <class TEntity>
auto& ServerEntityManagerHelper<TEntity>::GetEntity(ServerType type)
{
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
