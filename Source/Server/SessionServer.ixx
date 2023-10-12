module;
#include "hv/hasync.h"
#include "hv/EventLoop.h"

#include <iostream>
export module SessionServer;

import BaseServer;

using namespace hv;
using namespace std;

export class SessionServer;

class SessionServer : public BaseServer
{
public:
    SessionServer();

	virtual bool Init(map<string, string> &param) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	inline DnServer* GetSSock(){return pSSock;};

	inline DnClient* GetCSock(){return pCSock;};

    void LoopEvent(function<void(EventLoopPtr)> func);

public:
    DnServer* pSSock;

	DnClient* pCSock;

    //Control By Self
    // map<string, SocketChannelPtr> mChild;
};

module:private;

SessionServer::SessionServer()
{
    // mChild.clear();
    pSSock = nullptr;
	pCSock = nullptr;
}

bool SessionServer::Init(map<string, string> &param)
{
	/*int port = stoi(param["port"]);
	int listenfd = pSSock->createsocket(port, param["ip"].c_str());*/
	pSSock = new DnServer;

	int port = 555;
	int listenfd = pSSock->createsocket(port);
	if (listenfd < 0)
	{
		cout << "createsocket error\n";
		return false;
	}

	printf("pSSock listen on port %d, listenfd=%d ...\n", port, listenfd);

	auto setting = make_shared<unpack_setting_t>();
	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = 4;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;
	pSSock->setUnpack(setting.get());
	pSSock->setThreadNum(4);
	return true;
}

bool SessionServer::Start()
{
	if(!pSSock)
	{
		cout << "Server not Initialed" <<endl;
		return false;
	}
	pSSock->start();
	return true;
}

bool SessionServer::Stop()
{
	pSSock->stop();
	hv::async::cleanup();
	return true;
}

void SessionServer::LoopEvent(function<void(EventLoopPtr)> func)
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