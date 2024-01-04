module;
#include <functional>
#include <assert.h>
export module ControlServerHelper;

import ControlServer;
import EntityManagerHelper;
import ServerEntity;

using namespace std;

class ControlServerHelper : public ControlServer
{
private:
	ControlServerHelper(){}
public:

	// ControlServerHelper* GetSelf(){ return nullptr;}

	EntityManagerHelper<ServerEntity>* GetEntityManager(){ return nullptr;}
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