module;
#include <compare>
#include <assert.h>
export module ControlServerHelper;

export import ControlServer;
export import DNServerProxyHelper;
export import ServerEntityManagerHelper;

using namespace std;

export class ControlServerHelper : public ControlServer
{
private:
	ControlServerHelper(){}
public:
	DNServerProxyHelper* GetSSock(){ return nullptr;}
	ServerEntityManagerHelper* GetServerEntityManager(){ return nullptr;}
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
