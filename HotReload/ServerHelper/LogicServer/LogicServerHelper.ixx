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
import DNServerProxyHelper;
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
	DNServerProxyHelper* GetSSock(){ return nullptr;}
	ServerEntityManagerHelper* GetServerEntityManager(){ return nullptr;}
	ClientEntityManagerHelper* GetClientEntityManager(){ return nullptr;}

	bool InitDatabase();

	string& GetCtlIp(){ return sCtlIp;}
	uint16_t& GetCtlPort(){ return iCtlPort;}

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
	
	return true;
}

void LogicServerHelper::ReClientEvent(const string& ip, uint16_t port)
{

	// auto ReClient = [=, this]()
	// {
	// 	reconn_setting_t *reconn_setting = pCSock->reconn_setting;
	// 	unpack_setting_t *unpack_setting = pCSock->unpack_setting;

	// 	auto onConnection = pCSock->onConnection;
	// 	auto onMessage = pCSock->onMessage;
	// 	pCSock->reconn_setting = nullptr;
	// 	pCSock->unpack_setting = nullptr;
	// 	shared_ptr<EventLoopThread> loopPtr = pCSock->pLoop;

	// 	pCSock->stop();
	// 	pCSock->~DNClientProxy();
	// 	pCSock->DNClientProxy::DNClientProxy();

	// 	pCSock->reconn_setting = reconn_setting;
	// 	pCSock->unpack_setting = unpack_setting;
	// 	pCSock->onConnection = onConnection;
	// 	pCSock->onMessage = onMessage;
	// 	pCSock->pLoop = loopPtr;

	// 	pCSock->createsocket(port, ip.c_str());
	// 	pCSock->start();
	// };

	// AddMsgTask(ReClient);

	MainPostMsg msg;

	msg.type = MainPostMsg::Command;
	msg.sCommand << "redirectClient" << endl;
	msg.sCommand << ip << endl << port;

	AddMsgTask(msg);
}