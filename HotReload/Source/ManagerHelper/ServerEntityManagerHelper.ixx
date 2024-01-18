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
    CastTEntity* GetEntity(const string& ip);
};

template <class TEntity>
template <class CastTEntity>
CastTEntity* ServerEntityManagerHelper<TEntity>::AddEntity(const SocketChannelPtr& channel, unsigned int entityId, ServerType serverType)
{
	TEntity* entity = nullptr;
	if (ServerEntityManager<TEntity>::mEntityMap.count(entityId))
	{
		entity = ServerEntityManager<TEntity>::mEntityMap[entityId];
	}
	else
	{
		entity = new TEntity;
		ServerEntityManager<TEntity>::mEntityMap.emplace(entityId, entity);
		channel->setContext(entity);

		auto castEntity = static_cast<CastTEntity*>(entity);
		auto base = castEntity->GetChild();
		base->SetID(entityId);
		base->SetSock(channel);

		castEntity->SetServerType(serverType);

		// if(!ServerEntityManager<TEntity>::mEntityMapList.count(serverType))
		// {
		// 	ServerEntityManager<TEntity>::mEntityMapList[serverType] = new list<TEntity*>();
		// }

		ServerEntityManager<TEntity>::mEntityMapList[serverType].emplace_back(entity);

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
		ServerEntityManager<TEntity>::mEntityMap.erase(castEntity->GetChild()->GetID());
		ServerEntityManager<TEntity>::mEntityMapList[castEntity->GetServerType()].remove(entity);
		delete entity;

	}
	
}

template <class TEntity>
template <class CastTEntity>
CastTEntity *ServerEntityManagerHelper<TEntity>::GetEntity(const string& ip)
{
	for(auto &[k,v]: ServerEntityManager<TEntity>::mEntityMap)
	{
		auto castObj = static_cast<CastTEntity*>(v);
		if(castObj->GetServerIp() == ip)
			return castObj;
	}
	return nullptr;
}