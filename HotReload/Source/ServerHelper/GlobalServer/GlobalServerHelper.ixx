module;
#include "hv/htime.h"
#include "hv/EventLoop.h"

#include <functional>
export module GlobalServerHelper;

import GlobalServer;
import DNTask;

using namespace hv;
using namespace std;

export class GlobalServerHelper : public GlobalServer
{
public:
	template <typename... Args>
	void RegistSelf(function<DNTaskVoid(Args...)> func);
};

static GlobalServerHelper* PGlobalServerHelper = nullptr;

export void SetGlobalServer(GlobalServer* server)
{
	PGlobalServerHelper = static_cast<GlobalServerHelper*>(server);
}

export GlobalServerHelper* GetGlobalServer()
{
	return PGlobalServerHelper;
}

template <typename... Args>
void GlobalServerHelper::RegistSelf(function<DNTaskVoid(Args...)> func)
{
	GetCSock()->loop()->setInterval(100, [func, this](TimerID timerID)
	{
		if (GetCSock()->channel->isConnected() && !GetCSock()->IsRegisted()) 
		{
			func(this);
		} 
		else 
		{
			GetCSock()->loop()->killTimer(timerID);
		}
	});
}