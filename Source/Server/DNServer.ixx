module;

#include "hv/TcpServer.h"
#include "hv/TcpClient.h"
#include "hv/EventLoop.h"
export module DNServer;

import DNTask;

using namespace std;

export enum class ServerType : unsigned char
{
    None,
    ControlServer,
    GlobalServer,
};

export class DNServer
{
public:
	DNServer();
	virtual ~DNServer();

public:

	virtual bool Init(map<string, string> &param) = 0;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) = 0;

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

    ServerType GetServerType(){return emServerType;}

	virtual void LoopEvent(function<void(hv::EventLoopPtr)> func){}


	
protected:
    ServerType emServerType;
};

export class DNServerProxy : public hv::TcpServer
{
public:
	DNServerProxy();
	~DNServerProxy();
};

export class DNClientProxy : public hv::TcpClient
{
public:
	DNClientProxy();
	~DNClientProxy();

	auto GetMsgId() { return ++iMsgId; }

	auto& GetMsgMap(){ return mMsgList; }

	bool IsRegisted(){return bIsRegisted;}
	void SetRegisted(bool isRegisted){bIsRegisted = isRegisted;}
private:
	// only oddnumber
	unsigned char iMsgId;
	// unordered_
	map<unsigned int, DNTask<void*>* > mMsgList;
	// status
	bool bIsRegisted;
};

module:private;

DNServer::DNServer()
{
    emServerType = ServerType::None;
}

DNServer::~DNServer()
{
}

DNServerProxy::DNServerProxy()
{
}

DNServerProxy::~DNServerProxy()
{
}

DNClientProxy::DNClientProxy()
{
	iMsgId = 0;
	mMsgList.clear();
	bIsRegisted = false;
}

DNClientProxy::~DNClientProxy()
{
	for(auto& [k,v] : mMsgList)
	{
		v->Destroy();
	}
		
	mMsgList.clear();
}
