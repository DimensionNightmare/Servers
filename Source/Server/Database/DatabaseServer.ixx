module;
#include <cstdint>
#include <thread>
#include <iostream>
#include <unordered_map>
#include "pqxx/connection"
#include "hv/hsocket.h"
#include "hv/EventLoopThread.h"

#include "StdMacro.h"
#include "Common/Common.pb.h"
export module DatabaseServer;

export import DNServer;
import DNServerProxy;
import DNClientProxy;
import Logger;
import Config.Server;

using namespace std;
using namespace hv;

export class DatabaseServer : public DNServer
{
public:
	DatabaseServer();

	~DatabaseServer();

	virtual bool Init() override;

	virtual void InitCmd(unordered_map<string, function<void(stringstream*)>>& cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

public: // dll override
	virtual DNClientProxy* GetCSock() { return pCSock.get(); }

protected: // dll proxy
	unique_ptr<DNClientProxy> pCSock;

	unordered_map<uint16_t, unique_ptr<pqxx::connection>> pSqlProxys;

	// record orgin info
	string sCtlIp;
	uint16_t iCtlPort = 0;
};

DatabaseServer::DatabaseServer()
{
	emServerType = ServerType::DatabaseServer;
}

DatabaseServer::~DatabaseServer()
{
	pCSock = nullptr;
	pSqlProxys.clear();
}

bool DatabaseServer::Init()
{
	string* value = GetLuanchConfigParam("byCtl");
	if (!value || !stoi(*value))
	{
		DNPrint(ErrCode_SrvByCtl, LoggerLevel::Error, nullptr);
		return false;
	}

	DNServer::Init();

	uint16_t port = 0;

	// connet ControlServer
	string* ctlPort = GetLuanchConfigParam("ctlPort");
	string* ctlIp = GetLuanchConfigParam("ctlIp");
	if (ctlPort && ctlIp && is_ipaddr(ctlIp->c_str()))
	{
		pCSock = make_unique<DNClientProxy>();

		pCSock->Init();

		port = stoi(*ctlPort);
		pCSock->createsocket(port, ctlIp->c_str());

		sCtlIp = *ctlIp;
		iCtlPort = port;
	}

	return true;
}

void DatabaseServer::InitCmd(unordered_map<string, function<void(stringstream*)>>& cmdMap)
{
	DNServer::InitCmd(cmdMap);
}

bool DatabaseServer::Start()
{
	if (pCSock) // client
	{
		pCSock->Start();
	}

	return true;
}

bool DatabaseServer::Stop()
{
	if (pCSock) // client
	{
		pCSock->End();
	}

	return true;
}

void DatabaseServer::Pause()
{
	// pCSock->Timer()->pause();

	LoopEvent([](EventLoopPtr loop)
		{
			loop->pause();
		});
}

void DatabaseServer::Resume()
{
	LoopEvent([](EventLoopPtr loop)
		{
			loop->resume();
		});

	// pCSock->Timer()->resume();
}

void DatabaseServer::LoopEvent(function<void(EventLoopPtr)> func)
{
	unordered_map<long, bool> looped;

	if (pCSock)
	{
		looped.clear();
		while (const EventLoopPtr& pLoop = pCSock->loop())
		{
			long id = pLoop->tid();
			if (!looped.count(id))
			{
				func(pLoop);
				looped[id];
			}
			else
			{
				break;
			}
		};
	}
}
