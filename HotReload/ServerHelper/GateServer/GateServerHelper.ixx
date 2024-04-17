module;
#include "S_Global.pb.h"

#include <assert.h>
export module GateServerHelper;

import GateServer;
import DNClientProxyHelper;
import ServerEntityManagerHelper;
import ProxyEntityManagerHelper;
import Entity;
import MessagePack;

using namespace std;
using namespace GMsg::S_Global;

export class GateServerHelper : public GateServer
{
private:
	GateServerHelper(){};
public:

	DNClientProxyHelper* GetCSock(){ return nullptr;}
	ServerEntityManagerHelper<ServerEntity>* GetEntityManager(){ return nullptr;}
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
	G2G_RetRegistSrv upLoad;
	upLoad.set_server_index(castObj->GetChild()->ID());
	upLoad.set_is_regist(false);
	binData.resize(upLoad.ByteSizeLong());
	upLoad.SerializeToArray(binData.data(), (int)binData.size());
	MessagePack(0, MsgDeal::Ret, upLoad.GetDescriptor()->full_name().c_str(), binData);

	DNClientProxyHelper* client = GetCSock();
	client->send(binData);
	
	GetEntityManager()->RemoveEntity(castObj->GetChild()->ID());
}

void GateServerHelper::ProxyEntityCloseEvent(Entity* entity)
{
	ProxyEntityHelper* castObj = static_cast<ProxyEntityHelper*>(entity);

	auto entityMan = GetProxyEntityManager();
	entityMan->RemoveEntity(castObj->GetChild()->ID());
}