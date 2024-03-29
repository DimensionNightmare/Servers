module;
#include "S_Common.pb.h"
#include "hv/Channel.h"

#include <assert.h>
export module GlobalServerHelper;

import GlobalServer;
import DNClientProxyHelper;
import DNServerProxyHelper;
import ServerEntityManagerHelper;

import ServerEntityHelper;
import MessagePack;

using namespace std;
using namespace hv;
using namespace GMsg::S_Common;

#define CastObj(entity) static_cast<ServerEntityHelper*>(entity)

export class GlobalServerHelper : public GlobalServer
{
private:
	GlobalServerHelper(){};
public:

	DNClientProxyHelper* GetCSock(){ return nullptr;}
	DNServerProxyHelper* GetSSock(){ return nullptr;}
	ServerEntityManagerHelper<ServerEntity>* GetEntityManager(){ return nullptr;}

	void UpdateServerGroup();

public:
	static void TickHeartbeat(hio_t* io);
};

static GlobalServerHelper* PGlobalServerHelper = nullptr;

export void SetGlobalServer(GlobalServer* server)
{
	PGlobalServerHelper = static_cast<GlobalServerHelper*>(server);
	assert(PGlobalServerHelper != nullptr);
}

export GlobalServerHelper* GetGlobalServer()
{
	return PGlobalServerHelper;
}


void GlobalServerHelper::UpdateServerGroup()
{
	auto entityMan = GetEntityManager();

	list<ServerEntity*> gates = entityMan->GetEntityByList(ServerType::GateServer);
	if(gates.size() <= 0)
	{
		return;
	}

	list<ServerEntity*>& dbs = entityMan->GetEntityByList(ServerType::DatabaseServer);
	list<ServerEntity*>& logics = entityMan->GetEntityByList(ServerType::LogicServer);

	// alloc gate
	COM_RetChangeCtlSrv retMsg;
	string binData;

	auto registControl = [&](ServerEntityHelper* beEntity, ServerEntityHelper* entity)
	{
		SocketChannelPtr channel = entity->GetChild()->GetSock();
		entity->LinkNode() = beEntity;
		entity->GetChild()->SetSock(nullptr);
		channel->setContext(nullptr);
		
		// sendData
		binData.clear();
		retMsg.set_ip(beEntity->ServerIp());
		retMsg.set_port(beEntity->ServerPort());
		binData.resize(retMsg.ByteSize());
		retMsg.SerializeToArray(binData.data(), binData.size());
		MessagePack(0, MsgDeal::Ret, retMsg.GetDescriptor()->full_name().c_str(), binData);
		channel->write(binData);
		
		// timer destory
		entity->GetChild()->TimerId() = GetSSock()->loop(0)->setTimeout(10000,
			std::bind(&ServerEntityManager<ServerEntity>::EntityTimeoutTimer, (ServerEntityManager<ServerEntity>*)entityMan, placeholders::_1));

		entityMan->AddCloseTimer(entity->GetChild()->ID(), entity->GetChild()->TimerId());
	};
	
	for(ServerEntity* it : gates)
	{
		ServerEntityHelper* gate = CastObj(it);
		if(gate->HasFlag(ServerEntityFlag::Locked))
		{
			continue;
		}

		list<ServerEntity*>& gatesDb = gate->GetMapLinkNode(ServerType::DatabaseServer);
		list<ServerEntity*>& gatesLogic = gate->GetMapLinkNode(ServerType::LogicServer);
		if(!dbs.empty() && gatesDb.size() < 1)
		{
			ServerEntity* ele = dbs.front();
			dbs.pop_front();
			ServerEntityHelper* entity = CastObj(ele);
			registControl(gate, entity);
			entityMan->UnMountEntity(entity->ServerEntityType(), ele);
			gatesDb.emplace_back(ele);
		}

		if(!logics.empty() && gatesLogic.size() < 1)
		{
			ServerEntity* ele = logics.front();
			logics.pop_front();
			ServerEntityHelper* entity = CastObj(ele);
			registControl(gate, entity);
			entityMan->UnMountEntity(entity->ServerEntityType(), ele);
			gatesLogic.emplace_back(ele);
		}

		if (gatesDb.size() && gatesLogic.size())
		{
			// UnMountEntity(gate->GetServerType(), it);
			gate->SetFlag(ServerEntityFlag::Locked);
		}
		
	}
}

void GlobalServerHelper::TickHeartbeat(hio_t* io)
{
	SocketChannel* channel = (SocketChannel*)hio_context(io);
	//Regist?
	if(!channel->context())
	{
		channel->close(true);
		return;
	}
};
