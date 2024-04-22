module;
#include <coroutine>
#include "hv/Channel.h"

#include "Server/S_Auth_Control.pb.h"
#include "Server/S_Control_Global.pb.h"
export module ControlMessage:ControlAuth;

import DNTask;
import MessagePack;
import ControlServerHelper;
import ServerEntityHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

export DNTaskVoid Msg_ReqAuthAccount(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
{
	C2A_ResAuthAccount response;

	ServerEntityHelper* entity = nullptr;
	list<ServerEntity*>& servList = GetControlServer()->GetServerEntityManager()->GetEntityByList(ServerType::GlobalServer);
	for(ServerEntity* it : servList)
	{
		ServerEntityHelper* castEntity = static_cast<ServerEntityHelper*>(it);

		if(castEntity->TimerId())
		{
			continue;
		}

		if(!entity)
		{
			entity = castEntity;
			continue;
		}

		if(castEntity->GetConnNum() < entity->GetConnNum())
		{
			entity = castEntity;
		}
	}

	string binData;

	if(!entity)
	{
		response.set_state_code(1);
	}
	else
	{
		// message change to global
		auto dataChannel = [&response]()->DNTask<Message>
		{
			co_return response;
		}();

		DNServerProxyHelper* server = GetControlServer()->GetSSock();
		uint32_t smsgId = server->GetMsgId();

		binData.resize(msg->ByteSize());
		msg->SerializeToArray(binData.data(), binData.size());
		MessagePack(smsgId, MsgDeal::Req, C2G_ReqAuthAccount::GetDescriptor()->full_name().c_str(), binData);
		
		{
			// wait data parse
			server->AddMsg(smsgId, &dataChannel);
			entity->GetSock()->write(binData);
			co_await dataChannel;
			if(dataChannel.HasFlag(DNTaskFlag::Timeout))
			{

			}
		}

		dataChannel.Destroy();
	}

	
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	
	channel->write(binData);

	co_return;
}