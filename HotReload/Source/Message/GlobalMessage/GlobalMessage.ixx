module;
#include "GlobalControl.pb.h"
#include "hv/Channel.h"

#include <iostream>
#include <map>
#include <functional>
export module GlobalMessage;

export import GlobalServer;
export import GlobalControl;

using namespace std;
using namespace hv;
using namespace google::protobuf;

static GlobalServer* PGlobalServer = nullptr;

export void SetGlobalServer(GlobalServer* server)
{
	PGlobalServer = server;
}

export GlobalServer* GetGlobalServer()
{
	return PGlobalServer;
}

export class GlobalMessageHandle
{
public:
	static void MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, string name, Message *msg);
	static void RegMsgHandle();
public:
	inline static map<string, function<void(const SocketChannelPtr &, unsigned int, Message *)>> MHandleMap;
};

module :private;

void GlobalMessageHandle::MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, string name, Message *msg)
{
	if (MHandleMap.contains(name))
	{
		MHandleMap[name](channel, msgId, msg);
	}
}

void GlobalMessageHandle::RegMsgHandle()
{
	// MHandleMap[ G2C_RegistSrv::GetDescriptor()->full_name()] = &Msg_RegistSrv;
}