module;
#include "Server/S_Global.pb.h"
export module GateServerHelper;

export import GateServer;
export import DNClientProxyHelper;
export import DNServerProxyHelper;
export import ServerEntityManagerHelper;
export import ProxyEntityManagerHelper;
import MessagePack;
import Entity;

using namespace std;
using namespace GMsg;

export class GateServerHelper : public GateServer
{
private:
	GateServerHelper() = delete;;
public:

	DNClientProxyHelper* GetCSock() { return nullptr; }
	DNServerProxyHelper* GetSSock() { return nullptr; }
	ServerEntityManagerHelper* GetServerEntityManager() { return nullptr; }
	ProxyEntityManagerHelper* GetProxyEntityManager() { return nullptr; }

	void ServerEntityCloseEvent(Entity* entity);
	void ProxyEntityCloseEvent(Entity* entity);
};

static GateServerHelper* PGateServerHelper = nullptr;

export void SetGateServer(GateServer* server)
{
	PGateServerHelper = static_cast<GateServerHelper*>(server);
}

export GateServerHelper* GetGateServer()
{
	return PGateServerHelper;
}

// send close to change socket
void GateServerHelper::ServerEntityCloseEvent(Entity* entity)
{
	ServerEntity* cEntity = static_cast<ServerEntity*>(entity);

	// up to Global
	string binData;
	g2G_RetRegistSrv retMsg;
	retMsg.set_server_index(cEntity->ID());
	retMsg.set_is_regist(false);
	retMsg.SerializeToString(&binData);
	MessagePack(0, MsgDeal::Ret, retMsg.GetDescriptor()->full_name().c_str(), binData);

	DNClientProxyHelper* client = GetCSock();
	client->send(binData);

	GetServerEntityManager()->RemoveEntity(cEntity->ID());
}

void GateServerHelper::ProxyEntityCloseEvent(Entity* entity)
{
	ProxyEntity* cEntity = static_cast<ProxyEntity*>(entity);

	ProxyEntityManagerHelper* entityMan = GetProxyEntityManager();
	entityMan->RemoveEntity(cEntity->ID());
}