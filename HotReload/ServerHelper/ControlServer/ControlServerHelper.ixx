module;
#include <compare>
#include <assert.h>
export module ControlServerHelper;

import ControlServer;
import DNServerProxyHelper;
import ServerEntityManagerHelper;

using namespace std;

export class ControlServerHelper : public ControlServer
{
private:
	ControlServerHelper(){}
public:
	DNServerProxyHelper* GetSSock(){ return nullptr;}
	ServerEntityManagerHelper<ServerEntity>* GetServerEntityManager(){ return nullptr;}
};

static ControlServerHelper* PControlServerHelper = nullptr;

export void SetControlServer(ControlServer* server)
{
	PControlServerHelper = static_cast<ControlServerHelper*>(server);
	assert(PControlServerHelper != nullptr);
}

export ControlServerHelper* GetControlServer()
{
	return PControlServerHelper;
}
