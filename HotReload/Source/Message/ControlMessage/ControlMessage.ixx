module;
#include "GlobalControl.pb.h"

#include "hv/Channel.h"

#include <iostream>
#include <map>
#include <functional>
export module ControlMessage;

export import ControlGlobal;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::GlobalControl;

export class ControlMessageHandle
{
public:
	static void MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, string name, Message *msg);
	static void RegMsgHandle();
public:
	inline static map<string, function<void(const SocketChannelPtr &, unsigned int, Message *)>> MHandleMap;
};

module :private;

void ControlMessageHandle::MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, string name, Message *msg)
{
	if (MHandleMap.contains(name))
	{
		MHandleMap[name](channel, msgId, msg);
	}
}

void ControlMessageHandle::RegMsgHandle()
{
	MHandleMap[ G2C_RegistSrv::GetDescriptor()->full_name()] = &Msg_RegistSrv;
}