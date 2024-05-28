module;
export module ControlServerHelper;

export import ControlServer;
export import DNServerProxyHelper;
export import ServerEntityManagerHelper;

using namespace std;

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
}

export ControlServerHelper* GetControlServer()
{
	return PControlServerHelper;
}
