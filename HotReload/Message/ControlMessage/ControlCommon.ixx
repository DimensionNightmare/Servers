module;
#include <cstdint>
#include "hv/Channel.h"

#include "Server/S_Common.pb.h"
export module ControlMessage:ControlCommon;

import DNTask;
import MessagePack;
import ControlServerHelper;

using namespace google::protobuf;
using namespace GMsg;
using namespace hv;
using namespace std;

// client request
export void Msg_ReqRegistSrv(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
{
	COM_ReqRegistSrv* requset = reinterpret_cast<COM_ReqRegistSrv*>(msg);
	COM_ResRegistSrv response;

	ServerEntityManagerHelper*  entityMan = GetControlServer()->GetServerEntityManager();

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
		entity->SetSock(channel);

		channel->setContext(entity);

		response.set_success(true);
		response.set_server_index(entity->ID());
	}

	string binData;
	binData.resize(response.ByteSizeLong());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	channel->write(binData);
}

export void Exe_RetHeartbeat(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
{
	COM_RetHeartbeat* requset = reinterpret_cast<COM_RetHeartbeat*>(msg);
}
