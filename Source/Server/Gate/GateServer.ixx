module;
#include "StdAfx.h"
#include "hv/EventLoop.h"
#include "hv/hsocket.h"

export module GateServer;

import DNServer;
import DNServerProxy;
import DNClientProxy;
import MessagePack;
import ServerEntityManager;
import ProxyEntityManager;

using namespace std;
using namespace hv;

export class GateServer : public DNServer
{
public:
	GateServer();

	~GateServer();

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

	virtual ServerEntityManager<ServerEntity>* GetEntityManager(){return pEntityMan;}
	virtual ProxyEntityManager<ProxyEntity>* GetProxyEntityManager(){return pProxyEntityMan;}

protected: // dll proxy
	DNServerProxy* pSSock;
	DNClientProxy* pCSock;

	ServerEntityManager<ServerEntity>* pEntityMan;
	ProxyEntityManager<ProxyEntity>* pProxyEntityMan;
};



GateServer::GateServer()
{
	emServerType = ServerType::GateServer;
	pSSock = nullptr;
	pCSock = nullptr;

	pEntityMan = nullptr;
	pProxyEntityMan = nullptr;
}

GateServer::~GateServer()
{
	Stop();

	delete pEntityMan;
	pEntityMan = nullptr;

	if(pSSock)
	{
		pSSock->setUnpack(nullptr);
		delete pSSock;
		pSSock = nullptr;
	}

	if(pCSock)
	{
		pCSock->setReconnect(nullptr);
		delete pCSock;
		pCSock = nullptr;
	}
}

bool GateServer::Init()
{
	string* value = GetLuanchConfigParam("byCtl");
	if(!value || !stoi(*value))
	{
		DNPrint(1, LoggerLevel::Error, nullptr);
		return false;
	}

	DNServer::Init();

	int port = 0;
	
	value = GetLuanchConfigParam("port");
	if(value)
	{
		port = stoi(*value);
	}
	
	pSSock = new DNServerProxy;
	pSSock->pLoop = make_shared<EventLoopThread>();

	int listenfd = pSSock->createsocket(port);
	if (listenfd < 0)
	{
		DNPrint(8, LoggerLevel::Error, nullptr);
		return false;
	}

	// if not set port mean need get port by self 
	if(port == 0)
	{
		struct sockaddr_in addr;
		socklen_t addrLen = sizeof(addr);
		if (getsockname(listenfd, (struct sockaddr*)&addr, &addrLen) < 0) 
		{
			DNPrint(9, LoggerLevel::Error, nullptr);
			return false;
		}

		pSSock->port = ntohs(addr.sin_port);
	}
	
	DNPrint(1, LoggerLevel::Debug, nullptr, pSSock->port, listenfd);

	unpack_setting_t* setting = new unpack_setting_t;
	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = MessagePacket::PackLenth;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;
	pSSock->setUnpack(setting);
	pSSock->setThreadNum(4);

	
	//connet ControlServer
	string* ctlPort = GetLuanchConfigParam("ctlPort");
	string* ctlIp = GetLuanchConfigParam("ctlIp");
	if(ctlPort && ctlIp && is_ipaddr(ctlIp->c_str()))
	{
		pCSock = new DNClientProxy;
		pCSock->pLoop = make_shared<EventLoopThread>();

		reconn_setting_t* reconn = new reconn_setting_t;
		reconn->min_delay = 1000;
		reconn->max_delay = 10000;
		reconn->delay_policy = 2;
		pCSock->setReconnect(reconn);
		port = stoi(*ctlPort);
		pCSock->createsocket(port, ctlIp->c_str());
		pCSock->setUnpack(setting);
	}
	
	pEntityMan = new ServerEntityManager<ServerEntity>;
	pEntityMan->Init();
	pProxyEntityMan = new ProxyEntityManager<ProxyEntity>;
	pProxyEntityMan->Init();

	return true;
}

void GateServer::InitCmd(map<string, function<void(stringstream *)>> &cmdMap)
{
}

bool GateServer::Start()
{
	if(!pSSock)
	{
		DNPrint(6, LoggerLevel::Error, nullptr);
		return false;
	}

	pSSock->pLoop->start();
	pSSock->start();

	if(pCSock) // client
	{
		pCSock->pLoop->start();
		pCSock->start();
	}

	return true;
}

bool GateServer::Stop()
{
	if(pSSock)
	{
		pSSock->pLoop->stop(true);
		pSSock->stop();
	}

	if(pCSock) // client
	{
		pCSock->pLoop->stop(true);
		pCSock->stop();
	}

	return true;
}

void GateServer::Pause()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->pause(); 
	});
}

void GateServer::Resume()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->resume(); 
	});
}

void GateServer::LoopEvent(function<void(EventLoopPtr)> func)
{
    map<long,EventLoopPtr> looped;
    if(pSSock)
	{
		while(EventLoopPtr pLoop = pSSock->loop())
		{
			long id = pLoop->tid();
			if(looped.find(id) == looped.end())
			{
				func(pLoop);
				looped[id] = pLoop;
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
		while(EventLoopPtr pLoop = pCSock->loop())
		{
			long id = pLoop->tid();
			if(looped.find(id) == looped.end())
			{
				func(pLoop);
				looped[id] = pLoop;
			}
			else
			{
				break;
			}
		};
	}
	
    
}