module;
#include "Common.pb.h"
#include "hv/Channel.h"

export module ControlCommon;

import DNTask;
import MessagePack;
import ControlServerHelper;
import ServerEntityHelper;

using namespace google::protobuf;
using namespace GMsg::Common;
using namespace hv;
using namespace std;

// client request
export void Msg_RegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_ReqRegistSrv* requset = (COM_ReqRegistSrv*)msg;
	COM_ResRegistSrv response;

	//exist?
	if (auto entity = channel->getContext<ServerEntityHelper>())
	{
		response.set_success(false);
	}

	else if (auto entity =  GetControlServer()->GetEntityManager()->GetEntity<ServerEntityHelper>(channel->peeraddr()))
	{
		response.set_success(false);
	}

	else if (auto entity = GetControlServer()->GetEntityManager()->AddEntity<ServerEntityHelper>(channel, channel->id()))
	{
		entity->SetServerType((ServerType)requset->server_type());
		auto base = entity->GetChild();
		base->SetID(channel->id());
		base->SetSock(channel);
		response.set_success(true);
	}
	
	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, "", binData);
	channel->write(binData);
}