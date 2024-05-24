module;
#include <cstdint>
#include "hv/EventLoop.h"
#include "hv/hsocket.h"
#include "hv/EventLoopThread.h"
#include "sw/redis++/redis++.h"

#include "StdAfx.h"
export module LogicServer;

export import DNServer;
import DNServerProxy;
import DNClientProxy;
import ServerEntityManager;
import ClientEntityManager;
import RoomEntityManager;

using namespace std;
using namespace hv;
using namespace sw::redis;

export class LogicServer : public DNServer
{
public:
	LogicServer();

	~LogicServer();

	virtual bool Init() override;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

public: // dll override
	virtual DNServerProxy* GetSSock(){return pSSock;}
	virtual DNClientProxy* GetCSock(){return pCSock;}

	virtual ServerEntityManager* GetServerEntityManager(){return pServerEntityMan;}
	virtual ClientEntityManager* GetClientEntityManager(){return pClientEntityMan;}
	

protected: // dll proxy
	DNServerProxy* pSSock;
	DNClientProxy* pCSock;

	ServerEntityManager* pServerEntityMan;
	ClientEntityManager* pClientEntityMan;
	RoomEntityManager* pRoomMan;

	// record orgin info
	string sCtlIp;
	uint16_t iCtlPort;

	// localdb
	Redis* pNoSqlProxy;
};



LogicServer::LogicServer()
{
	emServerType = ServerType::LogicServer;
	pSSock = nullptr;
	pCSock = nullptr;

	pServerEntityMan = nullptr;
	pClientEntityMan = nullptr;
	pRoomMan = nullptr;

	pNoSqlProxy = nullptr;
}

LogicServer::~LogicServer()
{
	delete pServerEntityMan;
	pServerEntityMan = nullptr;

	if(pCSock)
	{
		pCSock->setReconnect(nullptr);
		delete pCSock;
		pCSock = nullptr;
	}

	if(pSSock)
	{
		pSSock->setUnpack(nullptr);
		delete pSSock;
		pSSock = nullptr;
	}
}

bool LogicServer::Init()
{
	string* value = GetLuanchConfigParam("byCtl");
	if(!value || !stoi(*value))
	{
		DNPrint(ErrCode_SrvByCtl, LoggerLevel::Error, nullptr);
		return false;
	}

	DNServer::Init();

	uint16_t port = 0;
	
	value = GetLuanchConfigParam("port");
	if(value)
	{
		port = stoi(*value);
	}
	
	pSSock = new DNServerProxy();
	pSSock->pLoop = make_shared<EventLoopThread>();

	int listenfd = pSSock->createsocket(port, "0.0.0.0");
	if (listenfd < 0)
	{
		DNPrint(ErrCode_CreateSocket, LoggerLevel::Error, nullptr);
		return false;
	}

	// if not set port mean need get port by self 
	if(port == 0)
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
	if(ctlPort && ctlIp && is_ipaddr(ctlIp->c_str()))
	{
		pCSock = new DNClientProxy();
		pCSock->pLoop = make_shared<EventLoopThread>();

		pCSock->Init();

		port = stoi(*ctlPort);
		pCSock->createsocket(port, ctlIp->c_str());

		sCtlIp = *ctlIp;
		iCtlPort = port;
	}
	
	pServerEntityMan = new ServerEntityManager();
	pServerEntityMan->Init();
	pClientEntityMan = new ClientEntityManager();
	pClientEntityMan->Init();
	pRoomMan = new RoomEntityManager();
	pRoomMan->Init();

	return true;
}

void LogicServer::InitCmd(map<string, function<void(stringstream *)>> &cmdMap)
{
	DNServer::InitCmd(cmdMap);

	/*** this cmd is import!
		if dll change client will use dll's codespace. 
		cant use main thread to exec lambda, it's unuseful.
		hv epoll has static Obj epoll__handle_tree, It have a bad effect.(connected‘s timing func dont exec)
	***/
	cmdMap.emplace("redirectClient", [this](stringstream* ss)
	{
		string ip;
		uint16_t port;
		*ss >> ip;
		*ss >> port;
		
		pCSock->RedirectClient(port, ip.c_str());
	});
}

bool LogicServer::Start()
{
	if(!pSSock)
	{
		DNPrint(ErrCode_SrvNotInit, LoggerLevel::Error, nullptr);
		return false;
	}

	pSSock->Start();

	if(pCSock) // client
	{
		pCSock->Start();
	}

	return true;
}

bool LogicServer::Stop()
{
	if(pCSock) // client
	{
		pCSock->End();
	}

	if(pSSock)
	{
		pSSock->End();
	}
	return true;
}

void LogicServer::Pause()
{
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
}

void LogicServer::LoopEvent(function<void(EventLoopPtr)> func)
{
    map<long,bool> looped;
    if(pSSock)
	{
		while(const EventLoopPtr& pLoop = pSSock->loop())
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

	if(pCSock)
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