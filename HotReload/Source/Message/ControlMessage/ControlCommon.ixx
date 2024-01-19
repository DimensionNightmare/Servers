module;
#include "Common.pb.h"
#include "hv/Channel.h"

export module ControlMessage:ControlCommon;

import DNTask;
import MessagePack;
import ControlServerHelper;
import ServerEntityHelper;
import DNServer;

using namespace google::protobuf;
using namespace GMsg::Common;
using namespace hv;
using namespace std;

// client request
export void EXE_Msg_RegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_ReqRegistSrv* requset = (COM_ReqRegistSrv*)msg;
	COM_ResRegistSrv response;

	auto entityMan = GetControlServer()->GetEntityManager();

	ServerType regType = (ServerType)requset->server_type();
	
	if(regType != ServerType::GlobalServer)
	{
		response.set_success(false);
	}

	//exist?
	else if (auto entity = channel->getContext<ServerEntityHelper>())
	{
		response.set_success(false);
	}

	else if (auto entity = entityMan->AddEntity<ServerEntityHelper>(channel, entityMan->GetServerIndex(), regType))
	{
		response.set_success(true);
	}

	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, "", binData);
	channel->write(binData);
}