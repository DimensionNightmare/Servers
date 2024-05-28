module;
#include "hv/EventLoop.h"
#include "hv/EventLoopThread.h"

#include "StdMacro.h"
#include "Common/Common.pb.h"
export module ControlServer;

export import DNServer;
import DNServerProxy;
import MessagePack;
import ServerEntityManager;
import Logger;
import Config.Server;

using namespace std;
using namespace hv;

export class ControlServer : public DNServer
{
public:
	ControlServer();

	~ControlServer();

	virtual bool Init() override;

	virtual void InitCmd(unordered_map<string, function<void(stringstream*)>>& cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

public: // dll override
	virtual DNServerProxy* GetSSock() { return pSSock.get(); }
	virtual ServerEntityManager* GetServerEntityManager() { return pServerEntityMan.get(); }

protected: // dll proxy
	unique_ptr<DNServerProxy> pSSock;

	unique_ptr<ServerEntityManager> pServerEntityMan;
};



ControlServer::ControlServer()
{
	emServerType = ServerType::ControlServer;
}

ControlServer::~ControlServer()
{
	pSSock = nullptr;
	pServerEntityMan = nullptr;
}

bool ControlServer::Init()
{
	string* port = GetLuanchConfigParam("port");
	if (!port)
	{
		DNPrint(ErrCode_SrvNeedIPPort, LoggerLevel::Error, nullptr);
		return false;
	}

	DNServer::Init();

	pSSock = make_unique<DNServerProxy>();

	int listenfd = pSSock->createsocket(stoi(*port), "0.0.0.0");
	if (listenfd < 0)
	{
		DNPrint(ErrCode_CreateSocket, LoggerLevel::Error, nullptr);
		return false;
	}

	DNPrint(TipCode_SrvListenOn, LoggerLevel::Normal, nullptr, pSSock->port, listenfd);

	pSSock->Init();

	pServerEntityMan = make_unique<ServerEntityManager>();
	pServerEntityMan->Init();

	return true;
}

void ControlServer::InitCmd(unordered_map<string, function<void(stringstream*)>>& cmdMap)
{
	
}

bool ControlServer::Start()
{
	if (!pSSock)
	{
		DNPrint(ErrCode_SrvNotInit, LoggerLevel::Error, nullptr);
		return false;
	}
	
	pSSock->Start();
	return true;
}

bool ControlServer::Stop()
{
	if (pSSock)
	{
		pSSock->End();
	}
	return true;
}

void ControlServer::Pause()
{
	// pSSock->Timer()->pause();
	// pServerEntityMan->Timer()->pause();

	LoopEvent([](EventLoopPtr loop)
	{ 
		loop->pause(); 
	});
}

void ControlServer::Resume()
{
	LoopEvent([](EventLoopPtr loop)
	{ 
		loop->resume(); 
	});

	// pSSock->Timer()->resume();
	// pServerEntityMan->Timer()->resume();
}

void ControlServer::LoopEvent(function<void(EventLoopPtr)> func)
{
	unordered_map<long, bool> looped;
	if (pSSock)
	{
		looped.clear();
		while (const EventLoopPtr& pLoop = pSSock->loop())
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
