module;
#include <assert.h>
#include <format>
#include "pqxx/connection"
#include "pqxx/transaction"
#include "hv/EventLoop.h"
#include "hv/hsocket.h"

#include "StdAfx.h"
#include "DbAfx.h"
export module LogicServerHelper;

import LogicServer;
import DNClientProxyHelper;
import ServerEntityManagerHelper;
import ClientEntityManagerHelper;

using namespace std;
using namespace hv;

export class LogicServerHelper : public LogicServer
{
private:
	LogicServerHelper(){};
public:

	DNClientProxyHelper* GetCSock(){ return nullptr;}
	ServerEntityManagerHelper<ServerEntity>* GetServerEntityManager(){ return nullptr;}
	ClientEntityManagerHelper<ClientEntity>* GetClientEntityManager(){ return nullptr;}

	bool InitDatabase();

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

bool LogicServerHelper::InitDatabase()
{
	
	return true;
}

void LogicServerHelper::ReClientEvent(const string& ip, unsigned short port)
{

	auto ReClient = [=, this]()
	{
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