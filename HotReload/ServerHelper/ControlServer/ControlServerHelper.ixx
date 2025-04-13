module;
export module ControlServerHelper;

import ControlServer;
import DNServerProxyHelper;
import ServerEntityManagerHelper;

export class ControlServerHelper : public ControlServer
{
private:

	ControlServerHelper() = delete;
public:

	DNServerProxyHelper* GetSSock() { return nullptr; }
	
	ServerEntityManagerHelper* GetServerEntityManager() { return nullptr; }
};

static ControlServerHelper* PControlServerHelper = nullptr;

export void SetControlServer(ControlServer* server)
{
	PControlServerHelper = static_cast<ControlServerHelper*>(server);
	if (!(PControlServerHelper != nullptr)) {abort();}
}

export ControlServerHelper* GetControlServer()
{
	return PControlServerHelper;
}
