module;
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

#include "StdMacro.h"
export module GateServer;

export import DNServer;
import DNServerProxy;
import DNClientProxy;
import ServerEntityManager;
import ProxyEntityManager;
import Logger;
import Config.Server;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export class GateServer : public DNServer
{
public:
	GateServer();

	~GateServer();

	virtual bool Init() override;

	virtual void InitCmd(unordered_map<string, function<void(stringstream*)>>& cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

public: // dll override
	virtual DNServerProxy* GetSSock() { return pSSock.get(); }
	virtual DNClientProxy* GetCSock() { return pCSock.get(); }

	virtual ServerEntityManager* GetServerEntityManager() { return pServerEntityMan.get(); }
	virtual ProxyEntityManager* GetProxyEntityManager() { return pProxyEntityMan.get(); }

protected: // dll proxy
	unique_ptr<DNServerProxy> pSSock;
	unique_ptr<DNClientProxy> pCSock;

	unique_ptr<ServerEntityManager> pServerEntityMan;
	unique_ptr<ProxyEntityManager> pProxyEntityMan;
};

GateServer::GateServer()
{
	emServerType = ServerType::GateServer;
}

// need init order reversal
GateServer::~GateServer()
{
	pSSock = nullptr;
	pCSock = nullptr;
	pServerEntityMan = nullptr;
	pProxyEntityMan = nullptr;
}

bool GateServer::Init()
{
	string* value = GetLuanchConfigParam("byCtl");
	if (!value || !stoi(*value))
	{
		DNPrint(ErrCode::ErrCode_SrvByCtl, LoggerLevel::Error, nullptr);
		return false;
	}

	DNServer::Init();

	uint16_t port = 0;

	value = GetLuanchConfigParam("port");
	if (value)
	{
		port = stoi(*value);
	}

	pSSock = make_unique<DNServerProxy>();

	int listenfd = pSSock->createsocket(port, "0.0.0.0");
	if (listenfd < 0)
	{
		DNPrint(ErrCode::ErrCode_CreateSocket, LoggerLevel::Error, nullptr);
		return false;
	}

	pSSock->Init();

	DNPrint(TipCode::TipCode_SrvListenOn, LoggerLevel::Normal, nullptr, pSSock->port, listenfd);

	//connet ControlServer
	string* ctlPort = GetLuanchConfigParam("ctlPort");
	string* ctlIp = GetLuanchConfigParam("ctlIp");
	if (ctlPort && ctlIp)
	{
		pCSock = make_unique<DNClientProxy>();

		pCSock->Init();

		port = stoi(*ctlPort);
		pCSock->createsocket(port, ctlIp->c_str());
	}

	pServerEntityMan = make_unique<ServerEntityManager>();
	pServerEntityMan->Init();
	pProxyEntityMan = make_unique<ProxyEntityManager>();
	pProxyEntityMan->Init();

	return true;
}

void GateServer::InitCmd(unordered_map<string, function<void(stringstream*)>>& cmdMap)
{
}

bool GateServer::Start()
{

	if (pCSock) // client
	{
		pCSock->Start();
	}

	if (!pSSock)
	{
		DNPrint(ErrCode::ErrCode_SrvNotInit, LoggerLevel::Error, nullptr);
		return false;
	}

	pSSock->Start();
	return true;
}

bool GateServer::Stop()
{
	if (pSSock)
	{
		pSSock->End();
	}

	if (pCSock) // client
	{
		pCSock->End();
	}

	return true;
}

void GateServer::Pause()
{
	// pSSock->Timer()->pause();
	// pCSock->Timer()->pause();
	// pServerEntityMan->Timer()->pause();

	LoopEvent([](EventLoopPtr loop)
		{
			loop->pause();
		});
}

void GateServer::Resume()
{
	LoopEvent([](EventLoopPtr loop)
		{
			loop->resume();
		});

	// pSSock->Timer()->resume();
	// pCSock->Timer()->resume();
	// pServerEntityMan->Timer()->resume();
}

void GateServer::LoopEvent(function<void(EventLoopPtr)> func)
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

	if (pCSock)
	{
		looped.clear();
		while (const EventLoopPtr& pLoop = pCSock->loop())
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
