module;
#include <format>
#include <cstdint>

#include "StdMacro.h"
export module LogicServerHelper;

export import LogicServer;
export import DNClientProxyHelper;
export import DNServerProxyHelper;
export import RoomEntityManagerHelper;
export import ClientEntityManagerHelper;
import Logger;
import Config.Server;
import ThirdParty.RedisPP;

export class LogicServerHelper : public LogicServer
{
private:
	LogicServerHelper() = delete;;
public:

	DNClientProxyHelper* GetCSock() { return nullptr; }
	DNServerProxyHelper* GetSSock() { return nullptr; }
	RoomEntityManagerHelper* GetRoomEntityManager() { return nullptr; }
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
	ASSERT(PLogicServerHelper != nullptr);
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
