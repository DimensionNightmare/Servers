module;
#include "hv/htime.h"
#include "hv/EventLoop.h"

#include <functional>
export module GlobalServerHelper;

import GlobalServer;
import DNTask;
import DNServerHelper;

using namespace hv;
using namespace std;

export class GlobalServerHelper : public GlobalServer
{
private:
	GlobalServerHelper(){};
public:
	// template <typename... Args>
	// void RegistSelf(function<DNTaskVoid(Args...)> func);

	DNClientProxyHelper* GetCSock(){ return nullptr;}
};

static GlobalServerHelper* PGlobalServerHelper = nullptr;

export void SetGlobalServer(GlobalServer* server)
{
	PGlobalServerHelper = static_cast<GlobalServerHelper*>(server);
	assert(PGlobalServerHelper != nullptr);
}

export GlobalServerHelper* GetGlobalServer()
{
	return PGlobalServerHelper;
}

// template <typename... Args>
// void GlobalServerHelper::RegistSelf(function<DNTaskVoid(Args...)> func)
// {
	
// }