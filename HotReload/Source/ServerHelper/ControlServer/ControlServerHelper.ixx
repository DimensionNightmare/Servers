module;

#include <assert.h>
export module ControlServerHelper;

import ControlServer;
import DNServerProxyHelper;
import EntityManagerControlHelper;
import ServerEntity;

using namespace std;

class ControlServerHelper : public ControlServer
{
private:
	ControlServerHelper(){}
public:
	DNServerProxyHelper* GetSSock(){ return nullptr;}
	EntityManagerControlHelper<ServerEntity>* GetEntityManager(){ return nullptr;}
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