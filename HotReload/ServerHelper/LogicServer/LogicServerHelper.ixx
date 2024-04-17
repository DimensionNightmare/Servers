module;
#include "StdAfx.h"
#include "DbAfx.h"
#include "pqxx/connection"
#include "pqxx/transaction"
#include "hv/EventLoop.h"
#include "hv/hsocket.h"

#include <assert.h>
#include <format>
export module LogicServerHelper;

import LogicServer;
import DNClientProxyHelper;
import ServerEntityManagerHelper;
import ServerEntity;

using namespace std;
using namespace hv;

export class LogicServerHelper : public LogicServer
{
private:
	LogicServerHelper(){};
public:

	DNClientProxyHelper* GetCSock(){ return nullptr;}
	ServerEntityManagerHelper<ServerEntity>* GetEntityManager(){ return nullptr;}

	bool InitDabase();

	string& GetCtlIp(){ return sCtlIp;}
	unsigned short& GetCtlPort(){ return iCtlPort;}

	void ReClientEvent(const string& ip, unsigned short port);
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

bool LogicServerHelper::InitDabase()
{
	try
	{
		//"postgresql://root@localhost"
		string* value = GetLuanchConfigParam("connection");
		pqxx::connection conn(*value);
		pqxx::work txn(conn);
		txn.commit();
	}
	catch(const exception& e)
	{
		DNPrint(-1, LoggerLevel::Error, "%s", e.what());
		return false;
	}

	return true;
}

void LogicServerHelper::ReClientEvent(const string& ip, unsigned short port)
{

	auto ReClient = [=, this]()
	{
		DNPrint(-1, LoggerLevel::Debug, "ThreadId:%d,Handle:%d", this_thread::get_id(), GetCurrentThread());

		reconn_setting_t *reconn_setting = pCSock->reconn_setting;
		unpack_setting_t *unpack_setting = pCSock->unpack_setting;

		pCSock->pLoop->stop();

		auto onConnection = pCSock->onConnection;
		auto onMessage = pCSock->onMessage;
		pCSock->reconn_setting = nullptr;
		pCSock->unpack_setting = nullptr;

		pCSock->stop();
		delete pCSock;
		pCSock = new DNClientProxy;
		pCSock->pLoop = make_shared<EventLoopThread>();

		pCSock->reconn_setting = reconn_setting;
		pCSock->unpack_setting = unpack_setting;
		pCSock->onConnection = onConnection;
		pCSock->onMessage = onMessage;

		pCSock->createsocket(port, ip.c_str());
		pCSock->pLoop->start();
		pCSock->start();
	};

	AddMsgTask(ReClient);
}