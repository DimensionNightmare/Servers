module;
#include <functional>
export module ControlServerHelper;

import ControlServer;

using namespace std;

class ControlServerHelper : public ControlServer
{
public:
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