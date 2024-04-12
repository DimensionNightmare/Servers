module;
#include "S_Auth.pb.h"
#include "hv/Channel.h"

#include <coroutine>
export module ControlMessage:ControlAuth;

import DNTask;
import MessagePack;
import ControlServerHelper;
import ServerEntityHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::S_Auth;

export DNTaskVoid Msg_ReqAuthAccount(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	G2A_ResAuthAccount response;

	ServerEntityHelper* entity = nullptr;
	list<ServerEntity*>& servList = GetControlServer()->GetEntityManager()->GetEntityByList(ServerType::GlobalServer);
	for(ServerEntity* it : servList)
	{
		ServerEntityHelper* castEntity = static_cast<ServerEntityHelper*>(it);

		if(castEntity->GetChild()->TimerId())
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
		// response.set_state_code(0);

		// message change to global
		auto dataChannel = [&response]()->DNTask<Message>
		{
			co_return response;
		}();

		DNServerProxyHelper* server = GetControlServer()->GetSSock();
		unsigned int smsgId = server->GetMsgId();

		binData.resize(msg->ByteSize());
		msg->SerializeToArray(binData.data(), binData.size());
		MessagePack(smsgId, MsgDeal::Req, msg->GetDescriptor()->full_name().c_str(), binData);
		
		{
			// wait data parse
			server->AddMsg(smsgId, &dataChannel);
			entity->GetChild()->GetSock()->write(binData);
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