module;

#include "GlobalControl.pb.h"
#include "hv/Channel.h"
export module ControlGlobal;

import DNTask;
import MessagePack;
import ControlServerHelper;

using namespace GMsg::GlobalControl;
using namespace google::protobuf;
using namespace hv;
using namespace std;

// client request
export void Msg_RegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	C2G_RegistSrv response;
	response.set_success(true);
	
	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, "", binData);
	auto entity = GetControlServer()->GetEntityManager()->AddEntity(channel, channel->id());
	entity->emServerType = ServerType::ControlServer;
	channel->write(binData);
}