module;
#include <cstdint>
#include <thread>
#include <iostream>
#include "hv/EventLoop.h"
#include "hv/hsocket.h"
#include "hv/EventLoopThread.h"

#include "StdAfx.h"
export module DatabaseServer;

export import DNServer;
import DNServerProxy;
import DNClientProxy;

using namespace std;
using namespace hv;

export class DatabaseServer : public DNServer
{
public:
	DatabaseServer();

	~DatabaseServer();

	virtual bool Init() override;

	virtual void InitCmd(map<string, function<void(stringstream *)>> &cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

public: // dll override
	virtual DNClientProxy *GetCSock() { return pCSock; }

protected: // dll proxy
	DNClientProxy *pCSock;

	// record orgin info
	string sCtlIp;
	uint16_t iCtlPort;
};

DatabaseServer::DatabaseServer()
{
	emServerType = ServerType::DatabaseServer;
	pCSock = nullptr;
}

DatabaseServer::~DatabaseServer()
{
	if (pCSock)
	{
		pCSock->setReconnect(nullptr);
		delete pCSock;
		pCSock = nullptr;
	}
}

bool DatabaseServer::Init()
{
	string *value = GetLuanchConfigParam("byCtl");
	if (!value || !stoi(*value))
	{
		DNPrint(ErrCode_SrvByCtl, LoggerLevel::Error, nullptr);
		return false;
	}

	DNServer::Init();

	uint16_t port = 0;

	// connet ControlServer
	string *ctlPort = GetLuanchConfigParam("ctlPort");
	string *ctlIp = GetLuanchConfigParam("ctlIp");
	if (ctlPort && ctlIp && is_ipaddr(ctlIp->c_str()))
	{
		pCSock = new DNClientProxy();
		pCSock->pLoop = make_shared<EventLoopThread>();

		pCSock->Init();

		port = stoi(*ctlPort);
		pCSock->createsocket(port, ctlIp->c_str());

		sCtlIp = *ctlIp;
		iCtlPort = port;
	}

	return true;
}

void DatabaseServer::InitCmd(map<string, function<void(stringstream *)>> &cmdMap)
{
	DNServer::InitCmd(cmdMap);
	
	cmdMap.emplace("redirectClient", [this](stringstream* ss)
	{
		string ip;
		uint16_t port;
		*ss >> ip;
		*ss >> port;

		pCSock->RedirectClient(port, ip.c_str());
	});
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
}

void DatabaseServer::LoopEvent(function<void(EventLoopPtr)> func)
{
	map<long, bool> looped;

	if (pCSock)
	{
		looped.clear();
		while(const EventLoopPtr& pLoop = pCSock->loop())
		{
			long id = pLoop->tid();
			if(!looped.count(id))
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
