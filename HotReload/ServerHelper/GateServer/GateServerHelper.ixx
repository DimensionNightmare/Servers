module;
#include <assert.h>

#include "Server/S_Global.pb.h"
export module GateServerHelper;

import GateServer;
import DNClientProxyHelper;
import DNServerProxyHelper;
import ServerEntityManagerHelper;
import ProxyEntityManagerHelper;
import MessagePack;

using namespace std;
using namespace GMsg::S_Global;

export class GateServerHelper : public GateServer
{
private:
	GateServerHelper(){};
public:

	DNClientProxyHelper* GetCSock(){ return nullptr;}
	DNServerProxyHelper* GetSSock(){ return nullptr;}
	ServerEntityManagerHelper<ServerEntity>* GetServerEntityManager(){ return nullptr;}
	ProxyEntityManagerHelper<ProxyEntity>* GetProxyEntityManager(){ return nullptr;}

	void ServerEntityCloseEvent(Entity *entity);
	void ProxyEntityCloseEvent(Entity *entity);
};

static GateServerHelper* PGateServerHelper = nullptr;

export void SetGateServer(GateServer* server)
{
	PGateServerHelper = static_cast<GateServerHelper*>(server);
	assert(PGateServerHelper != nullptr);
}

export GateServerHelper* GetGateServer()
{
	return PGateServerHelper;
}

// send close to change socket
void GateServerHelper::ServerEntityCloseEvent(Entity* entity)
{
	ServerEntityHelper* castObj = static_cast<ServerEntityHelper*>(entity);

	// up to Global
	string binData;
	G2G_RetRegistSrv retMsg;
	retMsg.set_server_index(castObj->ID());
	retMsg.set_is_regist(false);
	binData.resize(retMsg.ByteSizeLong());
	retMsg.SerializeToArray(binData.data(), (int)binData.size());
	MessagePack(0, MsgDeal::Ret, retMsg.GetDescriptor()->full_name().c_str(), binData);

	DNClientProxyHelper* client = GetCSock();
	client->send(binData);
	
	GetServerEntityManager()->RemoveEntity(castObj->ID());
}

void GateServerHelper::ProxyEntityCloseEvent(Entity* entity)
{
	ProxyEntityHelper* castObj = static_cast<ProxyEntityHelper*>(entity);

	auto entityMan = GetProxyEntityManager();
	entityMan->RemoveEntity(castObj->ID());
}