module;
#include "StdMacro.h"
export module ControlServer;

export import DNServer;
import DNServerProxy;
import MessagePack;
import ServerEntityManager;
import Logger;
import Config.Server;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

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

// need init order reversal
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
		DNPrint(ErrCode::ErrCode_SrvNeedIPPort, LoggerLevel::Error, nullptr);
		return false;
	}

	DNServer::Init();

	pSSock = make_unique<DNServerProxy>();

	int listenfd = pSSock->createsocket(stoi(*port), "0.0.0.0");
	if (listenfd < 0)
	{
		DNPrint(ErrCode::ErrCode_CreateSocket, LoggerLevel::Error, nullptr);
		return false;
	}

	pSSock->Init();

	DNPrint(TipCode::TipCode_SrvListenOn, LoggerLevel::Normal, nullptr, pSSock->port, listenfd);

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
		DNPrint(ErrCode::ErrCode_SrvNotInit, LoggerLevel::Error, nullptr);
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
			if (!looped.contains(id))
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
