module;
#include <cstdint>
#include "hv/EventLoop.h"
#include "hv/hsocket.h"
#include "hv/EventLoopThread.h"
#include "sw/redis++/redis++.h"

#include "StdMacro.h"
#include "Common/Common.pb.h"
export module LogicServer;

export import DNServer;
import DNServerProxy;
import DNClientProxy;
import ServerEntityManager;
import ClientEntityManager;
import RoomEntityManager;
import Logger;
import Config.Server;

using namespace std;
using namespace hv;
using namespace sw::redis;

export class LogicServer : public DNServer
{
public:
	LogicServer();

	~LogicServer();

	virtual bool Init() override;

	virtual void InitCmd(unordered_map<string, function<void(stringstream*)>>& cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

	virtual void TickMainFrame() override;

public: // dll override
	virtual DNServerProxy* GetSSock() { return pSSock.get(); }
	virtual DNClientProxy* GetCSock() { return pCSock.get(); }

	virtual ServerEntityManager* GetServerEntityManager() { return pServerEntityMan.get(); }
	virtual ClientEntityManager* GetClientEntityManager() { return pClientEntityMan.get(); }


protected: // dll proxy
	unique_ptr<DNServerProxy> pSSock;
	unique_ptr<DNClientProxy> pCSock;

	unique_ptr<ServerEntityManager> pServerEntityMan;
	unique_ptr<ClientEntityManager> pClientEntityMan;
	unique_ptr<RoomEntityManager> pRoomMan;

	// record orgin info
	string sCtlIp;
	uint16_t iCtlPort = 0;

	// localdb
	shared_ptr<Redis> pNoSqlProxy;
};



LogicServer::LogicServer()
{
	emServerType = ServerType::LogicServer;
}

// need init order reversal
LogicServer::~LogicServer()
{
	pSSock = nullptr;
	pCSock = nullptr;
	pRoomMan = nullptr;
	pClientEntityMan = nullptr;
	pServerEntityMan = nullptr;
	pNoSqlProxy = nullptr;
}

bool LogicServer::Init()
{
	string* value = GetLuanchConfigParam("byCtl");
	if (!value || !stoi(*value))
	{
		DNPrint(ErrCode_SrvByCtl, LoggerLevel::Error, nullptr);
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

		sCtlIp = *ctlIp;
		iCtlPort = port;
	}

	pServerEntityMan = make_unique<ServerEntityManager>();
	pServerEntityMan->Init();
	pClientEntityMan = make_unique<ClientEntityManager>();
	pClientEntityMan->Init();
	pRoomMan = make_unique<RoomEntityManager>();
	pRoomMan->Init();

	return true;
}

void LogicServer::InitCmd(unordered_map<string, function<void(stringstream*)>>& cmdMap)
{
	DNServer::InitCmd(cmdMap);

}

bool LogicServer::Start()
{
	if (!pSSock)
	{
		DNPrint(ErrCode_SrvNotInit, LoggerLevel::Error, nullptr);
		return false;
	}

	pSSock->Start();

	if (pCSock) // client
	{
		pCSock->Start();
	}

	return true;
}

bool LogicServer::Stop()
{
	if (pCSock) // client
	{
		pCSock->End();
	}

	if (pSSock)
	{
		pSSock->End();
	}
	return true;
}

void LogicServer::Pause()
{
	// pSSock->Timer()->pause();
	// pCSock->Timer()->pause();
	// pServerEntityMan->Timer()->pause();
	// pClientEntityMan->Timer()->pause();
	// pRoomMan->Timer()->pause();

	LoopEvent([](EventLoopPtr loop)
		{
			loop->pause();
		});
}

void LogicServer::Resume()
{
	LoopEvent([](EventLoopPtr loop)
		{
			loop->resume();
		});

	// pSSock->Timer()->resume();
	// pCSock->Timer()->resume();
	// pServerEntityMan->Timer()->resume();
	// pClientEntityMan->Timer()->resume();
	// pRoomMan->Timer()->resume();
}

void LogicServer::LoopEvent(function<void(EventLoopPtr)> func)
{
	unordered_map<long, bool> looped;
	if (pSSock)
	{
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

void LogicServer::TickMainFrame()
{
	pClientEntityMan->TickMainFrame();
}
