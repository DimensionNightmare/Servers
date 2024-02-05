module;
#include "Common.pb.h"
#include "hv/Channel.h"

#include <map>
#include <shared_mutex>
export module ServerEntityManagerHelper;

import ServerEntity;
import ServerEntityHelper;
import ServerEntityManager;
import MessagePack;
import DNServerProxy;

using namespace std;
using namespace hv;
using namespace GMsg::Common;

#define CastObj(entity) static_cast<ServerEntityHelper*>(entity)

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

	void UpdateServerGroup(DNServerProxy* sSock);
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
		
		entity = CastObj(oriEntity);
		entity->GetChild()->SetID(entityId);
		entity->SetServerType(regType);
	}

	return entity;
}

template <class TEntity>
void ServerEntityManagerHelper<TEntity>::RemoveEntity(unsigned int entityId, bool isDel)
{
	
	if(this->mEntityMap.count(entityId))
	{
		TEntity* oriEntity = this->mEntityMap[entityId];
		ServerEntityHelper* entity = CastObj(oriEntity);
		if(isDel)
		{
			unique_lock<shared_mutex> ulock(this->oMapMutex);

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
ServerEntityHelper* ServerEntityManagerHelper<TEntity>::GetEntity(unsigned int entityId)
{
	shared_lock<shared_mutex> lock(this->oMapMutex);
	ServerEntityHelper* entity = nullptr;
	if(this->mEntityMap.count(entityId))
	{
		TEntity* oriEntity = this->mEntityMap[entityId];
		return CastObj(oriEntity);
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
	if(this->mIdleServerId.size() > 0)
	{
		unsigned int index = this->mIdleServerId.front();
		this->mIdleServerId.pop_front();
		return index;
	}
	return this->iServerId++;
}

template <class TEntity>
void ServerEntityManagerHelper<TEntity>::UpdateServerGroup(DNServerProxy* sSock)
{
	list<TEntity*>& gates = GetEntityByList(ServerType::GateServer);
	if(gates.size() <= 0)
	{
		return;
	}

	list<TEntity*>& dbs = GetEntityByList(ServerType::DatabaseServer);
	list<TEntity*>& logics = GetEntityByList(ServerType::LogicServer);

	// alloc gate
	COM_RetChangeCtlSrv retMsg;
	string binData;

	auto registControl = [&](ServerEntityHelper* beEntity, ServerEntityHelper* entity)
	{
		SocketChannelPtr channel = entity->GetChild()->GetSock();
		entity->SetLinkNode(beEntity);
		entity->GetChild()->SetSock(nullptr);
		channel->setContext(nullptr);
		
		// sendData
		binData.clear();
		retMsg.set_ip(beEntity->GetServerIp());
		retMsg.set_port(beEntity->GetServerPort());
		binData.resize(retMsg.ByteSize());
		retMsg.SerializeToArray(binData.data(), binData.size());
		MessagePack(0, MsgDeal::Req, retMsg.GetDescriptor()->full_name(), binData);
		channel->write(binData);
		
		// timer destory
		uint64_t timerId = sSock->loop(0)->setTimeout(10000, [this, entity](uint64_t timerId)
		{
			RemoveEntity(entity->GetChild()->GetID());
		});

		entity->GetChild()->SetTimerId(timerId);
	};
	
	for(TEntity* it : gates)
	{
		ServerEntityHelper* gate = CastObj(it);
		list<ServerEntity*>& gatesDb = gate->GetMapLinkNode(ServerType::DatabaseServer);
		list<ServerEntity*>& gatesLogic = gate->GetMapLinkNode(ServerType::LogicServer);
		if(!dbs.empty() && gatesDb.size() < 1)
		{
			TEntity* ele = dbs.front();
			dbs.pop_front();
			ServerEntityHelper* entity = CastObj(ele);
			registControl(gate, entity);
			UnMountEntity(entity->GetServerType(), ele);
			gatesDb.emplace_back(ele);
		}

		if(!logics.empty() && gatesLogic.size() < 1)
		{
			TEntity* ele = logics.front();
			logics.pop_front();
			ServerEntityHelper* entity = CastObj(ele);
			registControl(gate, entity);
			UnMountEntity(entity->GetServerType(), ele);
			gatesLogic.emplace_back(ele);
		}

		if (gatesDb.size() && gatesLogic.size())
		{
			UnMountEntity(gate->GetServerType(), it);
		}
		
	}
}