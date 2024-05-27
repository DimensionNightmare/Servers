module;
#include <assert.h>
#include <format>
#include <cstdint>
#include "hv/EventLoop.h"
#include "hv/hsocket.h"
#include "sw/redis++/redis++.h"

#include "StdMacro.h"

export module LogicServerHelper;

export import LogicServer;
export import DNClientProxyHelper;
export import DNServerProxyHelper;
export import ServerEntityManagerHelper;
export import ClientEntityManagerHelper;
import Logger;
import Config.Server;
import Macro;

using namespace std;
using namespace hv;
using namespace sw::redis;

export class LogicServerHelper : public LogicServer
{
private:
	LogicServerHelper() = delete;;
public:

	DNClientProxyHelper* GetCSock() { return nullptr; }
	DNServerProxyHelper* GetSSock() { return nullptr; }
	ServerEntityManagerHelper* GetServerEntityManager() { return nullptr; }
	ClientEntityManagerHelper* GetClientEntityManager() { return nullptr; }

	bool InitDatabase();

	string& GetCtlIp() { return sCtlIp; }
	uint16_t& GetCtlPort() { return iCtlPort; }

	void ReClientEvent(const string& ip, uint16_t port);
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

bool LogicServerHelper::InitDatabase()
{
	if (pNoSqlProxy)
	{
		return true;
	}

	if (string* value = GetLuanchConfigParam("connection"))
	{
		try
		{
			pNoSqlProxy = make_unique<Redis>(*value);
			pNoSqlProxy->ping();
		}
		catch (const exception& e)
		{
			DNPrint(0, LoggerLevel::Debug, "%s", e.what());
			return false;
		}
	}

	return true;
}

void LogicServerHelper::ReClientEvent(const string& ip, uint16_t port)
{
	DNClientProxy* client = GetCSock();
	TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, RedirectClient, client, port, ip);
}