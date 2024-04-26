module;
#include "hv/EventLoop.h"
#include "hv/hsocket.h"

#include "StdAfx.h"
export module GlobalServer;

import DNServer;
import DNServerProxy;
import DNClientProxy;
import MessagePack;
import ServerEntityManager;

using namespace std;
using namespace hv;

export class GlobalServer : public DNServer
{
public:
	GlobalServer();

	~GlobalServer();

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

protected: // dll proxy
	DNServerProxy* pSSock;
	DNClientProxy* pCSock;

	ServerEntityManager* pServerEntityMan;
};



GlobalServer::GlobalServer()
{
	emServerType = ServerType::GlobalServer;
	pSSock = nullptr;
	pCSock = nullptr;

	pServerEntityMan = nullptr;
}

GlobalServer::~GlobalServer()
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

bool GlobalServer::Init()
{
	DNServer::Init();
	
	uint16_t port = 0;
	
	string* value = GetLuanchConfigParam("port");
	if(value)
	{
		port = stoi(*value);
	}
	
	pSSock = new DNServerProxy;
	pSSock->pLoop = make_shared<EventLoopThread>();

	int listenfd = pSSock->createsocket(port);
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

		pCSock->channel->setHeartbeat(4000, std::bind(&DNClientProxy::TickHeartbeat, pCSock));
		pCSock->channel->setWriteTimeout(12000);
	}
	
	pServerEntityMan = new ServerEntityManager;
	pServerEntityMan->Init();

	return true;
}

void GlobalServer::InitCmd(map<string, function<void(stringstream *)>> &cmdMap)
{
}

bool GlobalServer::Start()
{
	if(pCSock) // client
	{
		pCSock->pLoop->start();
		pCSock->start();
	}

	if(!pSSock)
	{
		DNPrint(ErrCode_SrvNotInit, LoggerLevel::Error, nullptr);
		return false;
	}

	pSSock->pLoop->start();
	pSSock->start();
	return true;
}

bool GlobalServer::Stop()
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

void GlobalServer::Pause()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->pause(); 
	});
}

void GlobalServer::Resume()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->resume(); 
	});
}

void GlobalServer::LoopEvent(function<void(EventLoopPtr)> func)
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