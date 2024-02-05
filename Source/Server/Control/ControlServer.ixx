module;
#include "hv/EventLoop.h"

export module ControlServer;

import DNServer;
import DNServerProxy;
import MessagePack;
import ServerEntity;
import ServerEntityManager;
import AfxCommon;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);

using namespace std;
using namespace hv;

export class ControlServer : public DNServer
{
public:
	ControlServer();

	~ControlServer();

	virtual bool Init(map<string, string> &param) override;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

public: // dll override
	virtual DNServerProxy* GetSSock(){return pSSock;}
	virtual ServerEntityManager<ServerEntity>* GetEntityManager(){return pEntityMan;}

protected: // dll proxy
	DNServerProxy* pSSock;

	ServerEntityManager<ServerEntity>* pEntityMan;
};

module:private;

ControlServer::ControlServer()
{
	emServerType = ServerType::ControlServer;
	pSSock = nullptr;
	pEntityMan = nullptr;
}

ControlServer::~ControlServer()
{
	Stop();
	
	delete pEntityMan;
	pEntityMan = nullptr;

	if(pSSock)
	{
		delete pSSock;
		pSSock = nullptr;
	}
}

bool ControlServer::Init(map<string, string> &param)
{
	if(!param.contains("ip") || !param.contains("port"))
	{
		DNPrintErr("ip or port Need! \n");
		return false;
	}

	DNServer::Init(param);

	int port = stoi(param["port"]);
	pSSock = new DNServerProxy;

	int listenfd = pSSock->createsocket(port);
	if (listenfd < 0)
	{
		DNPrintErr("createsocket error \n");
		return false;
	}

    // 输出分配的端口号
	DNPrint("pSSock listen on port %d, listenfd=%d ... \n", pSSock->port, listenfd);

	unpack_setting_t* setting = new unpack_setting_t;
	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = MessagePacket::PackLenth;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;
	pSSock->setUnpack(setting);
	pSSock->setThreadNum(4);

	pEntityMan = new ServerEntityManager<ServerEntity>;

	return true;
}

void ControlServer::InitCmd(map<string, function<void(stringstream *)>> &cmdMap)
{
	
}

bool ControlServer::Start()
{
	if(!pSSock)
	{
		DNPrintErr("Server not Initialed! \n");
		return false;
	}
	
	pSSock->start();
	return true;
}

bool ControlServer::Stop()
{
	if(pSSock)
	{
		pSSock->stop();
	}
	return true;
}

void ControlServer::Pause()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->pause(); 
	});
}

void ControlServer::Resume()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->resume(); 
	});
}

void ControlServer::LoopEvent(function<void(EventLoopPtr)> func)
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
}