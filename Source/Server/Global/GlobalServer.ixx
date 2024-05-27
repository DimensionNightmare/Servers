module;
#include <cstdint>
#include "hv/EventLoop.h"
#include "hv/hsocket.h"
#include "hv/EventLoopThread.h"

#include "StdMacro.h"
#include "Common/Common.pb.h"
export module GlobalServer;

export import DNServer;
import DNServerProxy;
import DNClientProxy;
import ServerEntityManager;
import Logger;
import Config.Server;

using namespace std;
using namespace hv;

export class GlobalServer : public DNServer
{
public:
	GlobalServer();

	~GlobalServer();

	virtual bool Init() override;

	virtual void InitCmd(map<string, function<void(stringstream*)>>& cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

public: // dll override
	virtual DNServerProxy* GetSSock() { return pSSock.get(); }
	virtual DNClientProxy* GetCSock() { return pCSock.get(); }

	virtual ServerEntityManager* GetServerEntityManager() { return pServerEntityMan.get(); }

protected: // dll proxy
	unique_ptr<DNServerProxy> pSSock;
	unique_ptr<DNClientProxy> pCSock;

	unique_ptr<ServerEntityManager> pServerEntityMan;
};



GlobalServer::GlobalServer()
{
	emServerType = ServerType::GlobalServer;
}

GlobalServer::~GlobalServer()
{
	pSSock = nullptr;
	pCSock = nullptr;
	pServerEntityMan = nullptr;
}

bool GlobalServer::Init()
{
	DNServer::Init();

	uint16_t port = 0;

	string* value = GetLuanchConfigParam("port");
	if (value)
	{
		port = stoi(*value);
	}

	pSSock = make_unique<DNServerProxy>();

	int listenfd = pSSock->createsocket(port, "0.0.0.0");
	if (listenfd < 0)
	{
		DNPrint(ErrCode_CreateSocket, LoggerLevel::Error, nullptr);
		return false;
	}

	// if not set port mean need get port by self 
	if (port == 0)
	{
		struct sockaddr_in addr;
		socklen_t addrLen = sizeof(addr);
		if (getsockname(listenfd, reinterpret_cast<struct sockaddr*>(&addr), &addrLen) < 0)
		{
			DNPrint(ErrCode_GetSocketName, LoggerLevel::Error, nullptr);
			return false;
		}

		pSSock->port = ntohs(addr.sin_port);
	}

	DNPrint(TipCode_SrvListenOn, LoggerLevel::Normal, nullptr, pSSock->port, listenfd);

	pSSock->Init();

	//connet ControlServer
	string* ctlPort = GetLuanchConfigParam("ctlPort");
	string* ctlIp = GetLuanchConfigParam("ctlIp");
	if (ctlPort && ctlIp && is_ipaddr(ctlIp->c_str()))
	{
		pCSock = make_unique<DNClientProxy>();

		pCSock->Init();

		port = stoi(*ctlPort);
		pCSock->createsocket(port, ctlIp->c_str());
	}

	pServerEntityMan = make_unique<ServerEntityManager>();
	pServerEntityMan->Init();

	return true;
}

void GlobalServer::InitCmd(map<string, function<void(stringstream*)>>& cmdMap)
{
}

bool GlobalServer::Start()
{
	if (pCSock) // client
	{
		pCSock->Start();
	}

	if (!pSSock)
	{
		DNPrint(ErrCode_SrvNotInit, LoggerLevel::Error, nullptr);
		return false;
	}

	pSSock->Start();
	return true;
}

bool GlobalServer::Stop()
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

void GlobalServer::Pause()
{
	// pSSock->Timer()->pause();
	// pCSock->Timer()->pause();
	// pServerEntityMan->Timer()->pause();

	LoopEvent([](EventLoopPtr loop)
		{
			loop->pause();
		});
}

void GlobalServer::Resume()
{
	LoopEvent([](EventLoopPtr loop)
		{
			loop->resume();
		});

	// pSSock->Timer()->resume();
	// pCSock->Timer()->resume();
	// pServerEntityMan->Timer()->resume();
}

void GlobalServer::LoopEvent(function<void(EventLoopPtr)> func)
{
	map<long, bool> looped;
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