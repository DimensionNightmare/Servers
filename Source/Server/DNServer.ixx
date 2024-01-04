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
	DNServer():emServerType(ServerType::None){};
	virtual ~DNServer(){};

public:

	virtual bool Init(map<string, string> &param) = 0;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) = 0;

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

    ServerType GetServerType(){return emServerType;}

	virtual void LoopEvent(function<void(hv::EventLoopPtr)> func){}

public: // dll override
	// virtual DNServer* GetSelf(){ return this;}

protected:
    ServerType emServerType;
};

export class DNServerProxy : public hv::TcpServer
{
public:
	DNServerProxy(){};
	~DNServerProxy(){};

public: // dll override
	// virtual DNServerProxy* GetSelf(){ return this;}
};

export class DNClientProxy : public hv::TcpClient
{
public:
	DNClientProxy();
	~DNClientProxy();

public: // dll override
	// virtual DNClientProxy* GetSelf(){ return this;}

protected: // dll proxy
	// only oddnumber
	unsigned char iMsgId;
	// unordered_
	map<unsigned int, DNTask<void*>* > mMsgList;
	// status
	bool bIsRegisted;
};

module:private;

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
