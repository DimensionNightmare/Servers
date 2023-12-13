module;
#include "hv/hasync.h"
#include "hv/hv.h"
#include "hv/EventLoop.h"

export module GlobalServer;

export import DNServer;
import MessagePack;

using namespace std;
using namespace hv;

export class GlobalServer : public DNServer
{
public:
	GlobalServer();

	~GlobalServer();

	virtual bool Init(map<string, string> &param) override;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;
	
	DNServerProxy* GetSSock(){return pSSock;}
	DNClientProxy* GetCSock(){return pCSock;}
private:
	DNServerProxy* pSSock;
	DNClientProxy* pCSock;
};

module:private;

GlobalServer::GlobalServer()
{
	emServerType = ServerType::GlobalServer;
	pSSock = nullptr;
	pCSock = nullptr;
}

GlobalServer::~GlobalServer()
{
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

bool GlobalServer::Init(map<string, string> &param)
{
	int port = 0;
	
	if(param.contains("port"))
	{
		port = stoi(param["port"]);
	}
	
	pSSock = new DNServerProxy;

	int listenfd = pSSock->createsocket(port);
	if (listenfd < 0)
	{
		fprintf(stderr, "%s->createsocket error! \n", __FUNCTION__);
		return false;
	}

	// if not set port mean need get port by self 
	if(port == 0)
	{
		struct sockaddr_in addr;
		socklen_t addrLen = sizeof(addr);
		if (getsockname(listenfd, (struct sockaddr*)&addr, &addrLen) < 0) {
			fprintf(stderr, "%s->Error in getsockname \n", __FUNCTION__);
			return false;
		}

		pSSock->port = ntohs(addr.sin_port);
	}
	
	printf("%s->pSSock listen on port %d, listenfd=%d ...\n", __FUNCTION__, pSSock->port, listenfd);

	auto setting = new unpack_setting_t;
	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = MessagePacket::PackLenth;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;
	pSSock->setUnpack(setting);
	pSSock->setThreadNum(4);

	
	//connet ControlServer
	if(stoi(param["byCtl"]) && param.contains("ctlPort") && param.contains("ctlIp"))
	{
		pCSock = new DNClientProxy;
		auto reconn = new reconn_setting_t;
		reconn->min_delay = 1000;
		reconn->max_delay = 10000;
		reconn->delay_policy = 2;
		pCSock->setReconnect(reconn);
		port = stoi(param["ctlPort"]);
		pCSock->createsocket(port, param["ctlIp"].c_str());
	}
	

	return true;
}

void GlobalServer::InitCmd(map<string, function<void(stringstream *)>> &cmdMap)
{
}

bool GlobalServer::Start()
{
	if(!pSSock)
	{
		fprintf(stderr, "%s->Server not Initialed! \n", __FUNCTION__);
		return false;
	}
	pSSock->start();

	if(pCSock)
	{
		pCSock->start();
	}

	return true;
}

bool GlobalServer::Stop()
{
	pSSock->stop();
	if(pCSock)
	{
		pCSock->stop();
	}
	hv::async::cleanup();
	return true;
}

void GlobalServer::LoopEvent(function<void(EventLoopPtr)> func)
{
    map<long,EventLoopPtr> looped;
    while(EventLoopPtr pLoop = pSSock->loop()){
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