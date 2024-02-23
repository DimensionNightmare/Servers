module;
#include "AuthControl.pb.h"
#include "GlobalControl.pb.h"
#include "hv/Channel.h"

#include <coroutine>
export module GlobalMessage:GlobalAuth;

import DNTask;
import MessagePack;
import GlobalServerHelper;
import ServerEntityHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::AuthControl;

export DNTaskVoid Exe_AuthAccount(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	A2C_AuthAccount* requset = (A2C_AuthAccount*)msg;
	C2A_AuthAccount response;


	co_return;
}