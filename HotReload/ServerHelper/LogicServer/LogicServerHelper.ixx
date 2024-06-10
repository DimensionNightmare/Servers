module;
#include <format>
#include <cstdint>
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

using namespace std;
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

	void ClearNosqlProxy() { pNoSqlProxy = nullptr; }
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
	if (string* value = GetLuanchConfigParam("connection"))
	{
		try
		{
			pNoSqlProxy = make_shared<Redis>(*value);
			pNoSqlProxy->ping();
		}
		catch (const exception& e)
		{
			DNPrint(0, LoggerLevel::Debug, "%s", e.what());
			return false;
		}
	}

	pClientEntityMan->InitSqlConn(pNoSqlProxy);

	return true;
}
