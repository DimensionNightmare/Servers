module;
#include "hv/TcpServer.h"
#include "hv/TcpClient.h"

#include "google/protobuf/message.h"
export module DNServer;

import DNTask;

using namespace std;
using namespace hv;
using namespace google::protobuf;

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

	virtual bool Init(map<string, string> &param) = 0;

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

    inline ServerType GetServerType(){return emServerType;}

	inline virtual void LoopEvent(function<void(EventLoopPtr)> func){}
	
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

	inline auto GetMsgId(){return iMsgId +=2;}

	inline auto GetMsgMap(){return &mMsgList;}

	// auto popMsg(int msgId);
private:
	// only oddnumber
	unsigned char iMsgId;

	map<int, pair<DNTaskVoid*, DNTask<Message*>*> > mMsgList;
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
	mMsgList.clear();
}