module;
#include "AuthControl.pb.h"
#include "GlobalControl.pb.h"
#include "hv/Channel.h"

export module ControlAuth;

import DNTask;
import MessagePack;
import ControlServerHelper;
import ServerEntityHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::AuthControl;

export void Msg_AuthAccount(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	A2C_AuthAccount* requset = (A2C_AuthAccount*)msg;
	C2A_AuthAccount response;

	
	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, "", binData);
	
	GetControlServer()->GetEntityManager();

	// GetControlServer()->GetSSock()->loop()->setTimeout(10000, [channel, binData](TimerID timerID) 
	// {
        channel->write(binData);
    // });

}