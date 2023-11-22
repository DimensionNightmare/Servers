module;
#include "google/protobuf/message.h"

#include "hv/TcpServer.h"
#include "hv/TcpClient.h"
#include "hv/htime.h"
#include "hv/EventLoop.h"
export module DNServer;

import DNTask;

using namespace std;
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

	inline virtual void LoopEvent(function<void(hv::EventLoopPtr)> func){}

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

	inline auto GetMsgId() { return ++iMsgId; }

	inline auto GetMsgMap(){ return &mMsgList; }

	template <typename... Args>
	inline void ExecTaskByDll(function<DNTaskVoid(Args...)> func, Args... args) { func(args...);}

	inline bool IsRegisted(){return bIsRegisted;}
	inline void SetRegisted(bool isRegisted){bIsRegisted = isRegisted;}

	template <typename... Args>
	void RegistSelf(function<DNTaskVoid(Args...)> func, Args... args);
private:
	// only oddnumber
	unsigned char iMsgId;
	// unordered_
	map<unsigned int, DNTask<Message*>* > mMsgList;
	// status
	bool bIsRegisted;
};

// template function can!t after module:private; impl !!!!!!!!!!!!!!!!
template <typename... Args>
void DNClientProxy::RegistSelf(function<DNTaskVoid(Args...)> func, Args... args)
{
	loop()->setInterval(3000, [func, args..., this](hv::TimerID timerID){
		if (channel->isConnected() && !IsRegisted()) {
			func(args...);
		} else {
			loop()->killTimer(timerID);
		}
	});
}

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