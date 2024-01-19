module;

#include <assert.h>
export module GateServerHelper;

import GateServer;
import DNClientProxyHelper;
import ServerEntityManagerHelper;
import ServerEntity;

using namespace std;

export class GateServerHelper : public GateServer
{
private:
	GateServerHelper(){};
public:

	DNClientProxyHelper* GetCSock(){ return nullptr;}
	ServerEntityManagerHelper<ServerEntity>* GetEntityManager(){ return nullptr;}
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