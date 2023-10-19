module;
#include "hv/hasync.h"
#include "hv/hv.h"
#include "hv/EventLoop.h"

#include <iostream>
export module GlobalServer;

import DNServer;
import MessagePack;

using namespace std;
using namespace hv;

export class GlobalServer : public DNServer
{
public:
	GlobalServer();

	virtual bool Init(map<string, string> &param) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

	inline virtual bool ClientSend(void* pData, int len) override;
	
	inline DNServerProxy* GetSSock(){return pSSock;}
	inline DNClientProxy* GetCSock(){return pCSock;}
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

bool GlobalServer::Init(map<string, string> &param)
{
	int port = 0;
	
	if(param.count("port") > 0)
	{
		port = stoi(param["port"]);
	}
	
	pSSock = new DNServerProxy;

	int listenfd = pSSock->createsocket(port);
	if (listenfd < 0)
	{
		cout << "createsocket error\n";
		return false;
	}

	if(port == 0)
	{
		struct sockaddr_in addr;
		socklen_t addrLen = sizeof(addr);
		if (getsockname(listenfd, (struct sockaddr*)&addr, &addrLen) < 0) {
			perror("Error in getsockname");
			return false;
		}

		pSSock->port = ntohs(addr.sin_port);
	}
	
	printf("pSSock listen on port %d, listenfd=%d ...\n", pSSock->port, listenfd);

	auto setting = make_shared<unpack_setting_t>();
	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = sizeof MessagePacket;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;
	pSSock->setUnpack(setting.get());
	pSSock->setThreadNum(4);

	
	//connet ControlServer
	if(atoi(param["byCtl"].c_str()) > 0 && param.count("ctlPort") > 0 && param.count("ctlIp") > 0)
	{
		pCSock = new DNClientProxy;
		auto reconn = make_shared<reconn_setting_t>();
		reconn->min_delay = 1000;
		reconn->max_delay = 10000;
		reconn->delay_policy = 2;
		pCSock->setReconnect(reconn.get());
		port = stoi(param["byCtl"]);
		pCSock->createsocket(port, param["ctlIp"].c_str());
	}
	

	return true;
}

bool GlobalServer::Start()
{
	if(!pSSock)
	{
		cout << "Server not Initialed" <<endl;
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

bool GlobalServer::ClientSend(void *pData, int len)
{

	// pCSock->writeBuf(pData, len);
	return false;
}
