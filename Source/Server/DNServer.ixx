module;
#include "hv/TcpServer.h"
#include "hv/TcpClient.h"

#include <any>
export module DNServer;

import DNTask;

using namespace std;
using namespace hv;

export enum class ServerType : int
{
    None,
    ControlServer,
    GlobalServer,
};

export class DNServer
{

public:
    DNServer();

	virtual bool Init(map<string, string> &param) = 0;

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

    inline ServerType GetServerType(){return emServerType;}

	inline virtual void LoopEvent(function<void(EventLoopPtr)> func){}

	virtual bool ClientSend(void* pData, int len) = 0;
	
protected:
    ServerType emServerType;
};

export class DNServerProxy : public TcpServer
{
public:
	DNServerProxy();
};

export class DNClientProxy : public TcpClient
{
public:
	DNClientProxy();

	inline auto GetMsgMap(){return &tasks;}
private:
	// only oddnumber
	unsigned int iMsgId;

	map<int, pair<DNTaskVoid*, DNTask<any*>* >> tasks;
};

module:private;

DNServer::DNServer()
{
    emServerType = ServerType::None;
}

DNServerProxy::DNServerProxy()
{
}

DNClientProxy::DNClientProxy()
{
	iMsgId = 1;
	tasks.clear();
}


