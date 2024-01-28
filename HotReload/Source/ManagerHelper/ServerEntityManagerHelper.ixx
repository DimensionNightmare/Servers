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
    auto AddEntity(unsigned int entityId, ServerType type);

    void RemoveEntity(unsigned int entityId, bool isDel = true);

	void MountEntity(ServerType type, TEntity* entity);

    void UnMountEntity(ServerType type, TEntity* entity);

    auto GetEntity(unsigned int id);

	auto& GetEntityByList(ServerType type);

	[[nodiscard]] unsigned int GetServerIndex();

	void UpdateServerGroup(DNServerProxy* sSock);
};

template <class TEntity>
auto ServerEntityManagerHelper<TEntity>::AddEntity(unsigned int entityId, ServerType regType)
{
	ServerEntityHelper* entity = nullptr;

	if (!this->mEntityMap.count(entityId))
	{
		unique_lock<shared_mutex> ulock(this->oMapMutex);

		auto oriEntity = new TEntity;
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
		auto oriEntity = this->mEntityMap[entityId];
		auto entity = CastObj(oriEntity);
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
auto ServerEntityManagerHelper<TEntity>::GetEntity(unsigned int entityId)
{
	shared_lock<shared_mutex> lock(this->oMapMutex);
	ServerEntityHelper* entity = nullptr;
	if(this->mEntityMap.count(entityId))
	{
		auto oriEntity = this->mEntityMap[entityId];
		return CastObj(oriEntity);
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

template <class TEntity>
void ServerEntityManagerHelper<TEntity>::UpdateServerGroup(DNServerProxy* sSock)
{
	auto gates = GetEntityByList(ServerType::GateServer);
	if(gates.size() <= 0)
		return;

	auto dbs = GetEntityByList(ServerType::DatabaseServer);
	auto logics = GetEntityByList(ServerType::LogicServer);

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
		auto timerId = sSock->loop(0)->setTimeout(10000, [this, entity](uint64_t timerId)
		{
			RemoveEntity(entity->GetChild()->GetID());
		});

		entity->GetChild()->SetTimerId(timerId);
	};
	
	for(auto it : gates)
	{
		auto gate = CastObj(it);
		auto& gatesDb = gate->GetMapLinkNode(ServerType::DatabaseServer);
		auto& gatesLogic = gate->GetMapLinkNode(ServerType::LogicServer);
		if(!dbs.empty() && gatesDb.size() < 1)
		{
			auto ele = dbs.front();
			dbs.pop_front();
			ServerEntityHelper* entity = CastObj(ele);
			registControl(gate, entity);
			UnMountEntity(entity->GetServerType(), ele);
			gatesDb.emplace_back(ele);
		}

		if(!logics.empty() && gatesLogic.size() < 1)
		{
			auto ele = logics.front();
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