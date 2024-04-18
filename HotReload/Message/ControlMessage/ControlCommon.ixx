module;
#include "S_Common.pb.h"

#include "hv/Channel.h"
export module ControlMessage:ControlCommon;

import DNTask;
import MessagePack;
import ControlServerHelper;
import ServerEntityHelper;
import ServerEntityManagerHelper;

using namespace google::protobuf;
using namespace GMsg::S_Common;
using namespace hv;
using namespace std;

// client request
export void Msg_ReqRegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_ReqRegistSrv* requset = reinterpret_cast<COM_ReqRegistSrv*>(msg);
	COM_ResRegistSrv response;

	ServerEntityManagerHelper<ServerEntity>*  entityMan = GetControlServer()->GetEntityManager();

	ServerType regType = (ServerType)requset->server_type();
	
	if(regType < ServerType::GlobalServer || regType > ServerType::AuthServer)
	{
		response.set_success(false);
	}

	//exist?
	else if (ServerEntityHelper* entity = channel->getContext<ServerEntityHelper>())
	{
		response.set_success(false);
	}

	else if (ServerEntityHelper* entity = entityMan->AddEntity(entityMan->ServerIndex(), regType))
	{
		entity->ServerIp() = requset->ip();
		entity->ServerPort() = requset->port();
		entity->GetChild()->SetSock(channel);

		channel->setContext(entity);

		response.set_success(true);
		response.set_server_index(entity->GetChild()->ID());
	}

	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	channel->write(binData);
}

export void Exe_RetHeartbeat(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_RetHeartbeat* requset = reinterpret_cast<COM_RetHeartbeat*>(msg);
}
