module;
#include "hv/EventLoop.h"
#include "hv/hsocket.h"

#include "StdAfx.h"
export module LogicServer;

import DNServer;
import DNServerProxy;
import DNClientProxy;
import MessagePack;
import ServerEntityManager;
import ClientEntityManager;
import RoomManager;

using namespace std;
using namespace hv;

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

	virtual ServerEntityManager<ServerEntity>* GetServerEntityManager(){return pServerEntityMan;}
	virtual ClientEntityManager<ClientEntity>* GetClientEntityManager(){return pClientEntityMan;}
	

protected: // dll proxy
	DNServerProxy* pSSock;
	DNClientProxy* pCSock;

	ServerEntityManager<ServerEntity>* pServerEntityMan;
	ClientEntityManager<ClientEntity>* pClientEntityMan;
	RoomManager<RoomEntity>* pRoomMan;

	// record orgin info
	string sCtlIp;
	uint16_t iCtlPort;
};



LogicServer::LogicServer()
{
	emServerType = ServerType::LogicServer;
	pSSock = nullptr;
	pCSock = nullptr;

	pServerEntityMan = nullptr;
	pClientEntityMan = nullptr;
	pRoomMan = nullptr;
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

		sCtlIp = *ctlIp;
		iCtlPort = port;
	}
	
	pServerEntityMan = new ServerEntityManager<ServerEntity>;
	pServerEntityMan->Init();
	pClientEntityMan = new ClientEntityManager<ClientEntity>;
	pClientEntityMan->Init();

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

		pCSock->createsocket(port, ip.c_str());
		pCSock->start();
	});
}

bool LogicServer::Start()
{
	if(!pSSock)
	{
		DNPrint(ErrCode_SrvNotInit, LoggerLevel::Error, nullptr);
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

bool LogicServer::Stop()
{
	if(pCSock) // client
	{
		pCSock->pLoop->stop(true);
		pCSock->stop();
	}

	if(pSSock)
	{
		pSSock->pLoop->stop(true);
		pSSock->stop();
	}
	return true;
}

void LogicServer::Pause()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->pause(); 
	});
}

void LogicServer::Resume()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->resume(); 
	});
}

void LogicServer::LoopEvent(function<void(EventLoopPtr)> func)
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