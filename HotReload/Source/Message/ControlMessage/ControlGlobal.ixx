module;

#include "hv/Channel.h"
#include "GlobalControl.pb.h"

#include <coroutine>
export module ControlGlobal;

import DNTask;
import DNServer;
import MessagePack;

using namespace GMsg::GlobalControl;
using namespace std;
using namespace google::protobuf;
using namespace hv;

// client request
export void Msg_RegistSrv(const SocketChannelPtr &channel, unsigned char msgId, Message *msg)
{
	C2G_RegistSrv response;
	response.set_success(true);
	
	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	string name;
	MessagePack(msgId, MsgDir::Inner, name, binData);
	channel->write(binData);
}