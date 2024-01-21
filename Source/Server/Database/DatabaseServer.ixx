module;
#include "hv/EventLoop.h"
#include "hv/hsocket.h"

export module DatabaseServer;

import DNServer;
import DNServerProxy;
import DNClientProxy;
import MessagePack;
import AfxCommon;
import ServerEntity;
import ServerEntityManager;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);

using namespace std;
using namespace hv;

export class DatabaseServer : public DNServer
{
public:
	DatabaseServer();

	~DatabaseServer();

	virtual bool Init(map<string, string> &param) override;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

	virtual void ReClientEvent(const char* ip, int port) override;

public: // dll override
	virtual DNServerProxy* GetSSock(){return pSSock;}
	virtual DNClientProxy* GetCSock(){return pCSock;}

protected: // dll proxy
	DNServerProxy* pSSock;
	DNClientProxy* pCSock;

	// record orgin info
	string sCtlIp;
	unsigned short iCtlPort;
};

module:private;

DatabaseServer::DatabaseServer()
{
	emServerType = ServerType::DatabaseServer;
	pSSock = nullptr;
	pCSock = nullptr;
}

DatabaseServer::~DatabaseServer()
{
	Stop();

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

bool DatabaseServer::Init(map<string, string> &param)
{
	if(!param.count("byCtl"))
	{
		DNPrintErr("Server need by Control! \n");
		return false;
	}

	DNServer::Init(param);

	int port = 0;
	
	if(param.contains("port"))
	{
		port = stoi(param["port"]);
	}
	
	pSSock = new DNServerProxy;

	int listenfd = pSSock->createsocket(port);
	if (listenfd < 0)
	{
		DNPrintErr("createsocket error! \n");
		return false;
	}

	// if not set port mean need get port by self 
	if(port == 0)
	{
		struct sockaddr_in addr;
		socklen_t addrLen = sizeof(addr);
		if (getsockname(listenfd, (struct sockaddr*)&addr, &addrLen) < 0) 
		{
			DNPrintErr("Error in getsockname \n");
			return false;
		}

		pSSock->port = ntohs(addr.sin_port);
	}
	
	DNPrint("pSSock listen on port %d, listenfd=%d ... \n", pSSock->port, listenfd);

	auto setting = new unpack_setting_t;
	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = MessagePacket::PackLenth;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;
	pSSock->setUnpack(setting);
	pSSock->setThreadNum(4);

	
	//connet ControlServer
	if(param.contains("ctlPort") && param.contains("ctlIp") && is_ipaddr(param["ctlIp"].c_str()))
	{
		pCSock = new DNClientProxy;
		auto reconn = new reconn_setting_t;
		reconn->min_delay = 1000;
		reconn->max_delay = 10000;
		reconn->delay_policy = 2;
		pCSock->setReconnect(reconn);
		port = stoi(param["ctlPort"]);
		pCSock->createsocket(port, param["ctlIp"].c_str());
		pCSock->setUnpack(setting);

		sCtlIp = param["ctlIp"];
		iCtlPort = port;
	}
	
	return true;
}

void DatabaseServer::InitCmd(map<string, function<void(stringstream *)>> &cmdMap)
{
}

bool DatabaseServer::Start()
{
	if(!pSSock)
	{
		DNPrintErr("Server not Initialed! \n");
		return false;
	}

	pSSock->start();

	if(pCSock) // client
	{
		pCSock->start();
	}

	return true;
}

bool DatabaseServer::Stop()
{
	if(pSSock)
	{
		pSSock->stop();
	}

	if(pCSock) // client
	{
		pCSock->stop();
	}

	return true;
}

void DatabaseServer::Pause()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->pause(); 
	});
}

void DatabaseServer::Resume()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->resume(); 
	});
}

void DatabaseServer::LoopEvent(function<void(EventLoopPtr)> func)
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

void DatabaseServer::ReClientEvent(const char* ip, int port)
{
	auto reconn_setting = new reconn_setting_t;
	memcpy(reconn_setting, pCSock->reconn_setting, sizeof reconn_setting);
	auto unpack_setting = pCSock->unpack_setting;
	auto onConnection = pCSock->onConnection;
	auto onMessage = pCSock->onMessage;

	pCSock->stop();
	// delete pCSock;
	pCSock = new DNClientProxy;

	pCSock->reconn_setting = reconn_setting;
	pCSock->unpack_setting = unpack_setting;
	pCSock->onConnection = onConnection;
	pCSock->onMessage = onMessage;

	pCSock->createsocket(port, ip);
	pCSock->start();
}
