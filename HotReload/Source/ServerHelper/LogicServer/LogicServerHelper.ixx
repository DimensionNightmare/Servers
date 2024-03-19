module;

#include <assert.h>
export module LogicServerHelper;

import LogicServer;
import DNClientProxyHelper;
import DNServerProxyHelper;
import ServerEntityManagerHelper;
import ServerEntity;

using namespace std;

export class LogicServerHelper : public LogicServer
{
private:
	LogicServerHelper(){};
public:

	DNClientProxyHelper* GetCSock(){ return nullptr;}
	DNServerProxyHelper* GetSSock(){ return nullptr;}
	
	ServerEntityManagerHelper<ServerEntity>* GetEntityManager(){ return nullptr;}

	string& GetCtlIp(){ return sCtlIp;}
	int GetCtlPort(){ return iCtlPort;}
};

static LogicServerHelper* PLogicServerHelper = nullptr;

export void SetLogicServer(LogicServer* server)
{
	PLogicServerHelper = static_cast<LogicServerHelper*>(server);
	assert(PLogicServerHelper != nullptr);
}

export LogicServerHelper* GetLogicServer()
{
	return PLogicServerHelper;
}