module;
#include "hv/TcpServer.h"
#include "hv/TcpClient.h"

#include "google/protobuf/message.h"

#include <mutex>

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

	virtual bool Init(map<string, string> &param) = 0;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) = 0;

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

    inline ServerType GetServerType(){return emServerType;}

	inline virtual void LoopEvent(function<void(EventLoopPtr)> func){}

protected:
	DNServer();
	
protected:
    ServerType emServerType;
};

export class DNServerProxy : public TcpServer
{
public:
	DNServerProxy();
};

mutex idMutex;

export class DNClientProxy : public TcpClient
{
public:
	DNClientProxy();

	inline auto GetMsgId()
	{
		std::lock_guard<std::mutex> lock(idMutex); 
		return ++iMsgId;
	}

	inline auto GetMsgMap(){return &mMsgList;}

	// auto popMsg(int msgId);
private:
	// only oddnumber
	unsigned int iMsgId;
	//unordered_
	map<unsigned int, DNTask<Message*>* > mMsgList;
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