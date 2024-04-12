module;

#include <assert.h>
export module ControlServerHelper;

import ControlServer;
import DNServerProxyHelper;
import ServerEntityManagerHelper;
import ServerEntity;

using namespace std;

export class ControlServerHelper : public ControlServer
{
private:
	ControlServerHelper(){}
public:
	DNServerProxyHelper* GetSSock(){ return nullptr;}
	ServerEntityManagerHelper<ServerEntity>* GetEntityManager(){ return nullptr;}
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
