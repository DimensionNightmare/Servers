module;

#include "hv/Channel.h"
#include "GlobalControl.pb.h"
export module ControlGlobal;

import DNTask;
import MessagePack;

using namespace GMsg::GlobalControl;
using namespace google::protobuf;
using namespace hv;
using namespace std;

// client request
export void Msg_RegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	C2G_RegistSrv response;
	response.set_success(false);
	
	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	string name;
	MessagePack(msgId, MsgDir::Inner, name, binData);
	channel->write(binData);
}