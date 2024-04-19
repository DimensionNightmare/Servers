module;
#include <assert.h>
#include <format>
#include "pqxx/connection"
#include "pqxx/transaction"
#include "hv/EventLoop.h"
#include "hv/hsocket.h"

#include "StdAfx.h"
#include "DbAfx.h"
export module DatabaseServerHelper;

import DatabaseServer;
import DNClientProxyHelper;
import ServerEntityManagerHelper;

using namespace std;
using namespace hv;

export class DatabaseServerHelper : public DatabaseServer
{
private:
	DatabaseServerHelper(){};
public:

	DNClientProxyHelper* GetCSock(){ return nullptr;}
	ServerEntityManagerHelper<ServerEntity>* GetServerEntityManager(){ return nullptr;}

	bool InitDatabase();

	string& GetCtlIp(){ return sCtlIp;}
	unsigned short& GetCtlPort(){ return iCtlPort;}

	void ReClientEvent(const string& ip, unsigned short port);
};

static DatabaseServerHelper* PDatabaseServerHelper = nullptr;

export void SetDatabaseServer(DatabaseServer* server)
{
	PDatabaseServerHelper = static_cast<DatabaseServerHelper*>(server);
	assert(PDatabaseServerHelper != nullptr);
}

export DatabaseServerHelper* GetDatabaseServer()
{
	return PDatabaseServerHelper;
}

bool DatabaseServerHelper::InitDatabase()
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

void DatabaseServerHelper::ReClientEvent(const string& ip, unsigned short port)
{

	auto ReClient = [=, this]()
	{
		reconn_setting_t *reconn_setting = pCSock->reconn_setting;
		unpack_setting_t *unpack_setting = pCSock->unpack_setting;

		auto onConnection = pCSock->onConnection;
		auto onMessage = pCSock->onMessage;
		pCSock->reconn_setting = nullptr;
		pCSock->unpack_setting = nullptr;
		shared_ptr<EventLoopThread> loopPtr = pCSock->pLoop;

		pCSock->stop();
		pCSock->~DNClientProxy();
		pCSock->DNClientProxy::DNClientProxy();

		pCSock->reconn_setting = reconn_setting;
		pCSock->unpack_setting = unpack_setting;
		pCSock->onConnection = onConnection;
		pCSock->onMessage = onMessage;
		pCSock->pLoop = loopPtr;

		pCSock->createsocket(port, ip.c_str());
		pCSock->start();
	};

	AddMsgTask(ReClient);
}