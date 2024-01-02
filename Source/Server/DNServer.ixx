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

	virtual bool Init(map<string, string> &param) = 0;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) = 0;

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

    ServerType GetServerType(){return emServerType;}

	virtual void LoopEvent(function<void(hv::EventLoopPtr)> func){}

protected:
	DNServer();
	
protected:
    ServerType emServerType;
};

export class DNServerProxy : public hv::TcpServer
{
public:
	DNServerProxy();
};

export class DNClientProxy : public hv::TcpClient
{
public:
	DNClientProxy();

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

DNServerProxy::DNServerProxy()
{
}

DNClientProxy::DNClientProxy()
{
	iMsgId = 0;
	mMsgList.clear();
	bIsRegisted = false;
}