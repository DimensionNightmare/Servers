module;
#include "hv/Channel.h"

#include <assert.h>
export module GlobalServerHelper;

import GlobalServer;
import DNClientProxyHelper;
import ServerEntityManagerHelper;
import ServerEntity;

import ServerEntityHelper;

using namespace std;
using namespace hv;

export class GlobalServerHelper : public GlobalServer
{
private:
	GlobalServerHelper(){};
public:

	DNClientProxyHelper* GetCSock(){ return nullptr;}
	ServerEntityManagerHelper<ServerEntity>* GetEntityManager(){ return nullptr;}
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