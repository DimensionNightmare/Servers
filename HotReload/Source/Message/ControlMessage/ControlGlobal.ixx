module;

#include "GlobalControl.pb.h"
#include "hv/Channel.h"
export module ControlGlobal;

import DNTask;
import MessagePack;
import ControlServerHelper;
import ServerEntityHelper;

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

	// cout << channel->peeraddr() << endl;

	MessagePack(msgId, MsgDeal::Res, "", binData);
	channel->write(binData);
	
	auto entity = GetControlServer()->GetEntityManager()->AddEntity<ServerEntityHelper>(channel, channel->id());
	entity->SetServerType(ServerType::ControlServer);
	auto base = entity->GetChild();
	base->SetID(channel->id());
	base->SetSock(channel);

}