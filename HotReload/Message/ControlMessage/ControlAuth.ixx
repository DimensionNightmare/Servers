module;
#include <coroutine>
#include <cstdint>
#include <list>
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Server/S_Auth_Control.pb.h"
#include "Server/S_Control_Global.pb.h"
export module ControlMessage:ControlAuth;

import DNTask;
import MessagePack;
import ControlServerHelper;
import Logger;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

namespace ControlMessage
{
	export DNTaskVoid Msg_ReqAuthAccount(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		C2A_ResAuthAccount response;

		ServerEntityHelper* entity = nullptr;
		list<ServerEntity*>& servList = GetControlServer()->GetServerEntityManager()->GetEntityByList(ServerType::GlobalServer);
		for (ServerEntity* it : servList)
		{
			ServerEntityHelper* castEntity = static_cast<ServerEntityHelper*>(it);

			if (castEntity->TimerId())
			{
				continue;
			}

			if (!entity)
			{
				entity = castEntity;
				continue;
			}

			if (castEntity->GetConnNum() < entity->GetConnNum())
			{
				entity = castEntity;
			}
		}

		string binData;

		if (!entity)
		{
			response.set_state_code(2);
		}
		else
		{

			DNServerProxyHelper* server = GetControlServer()->GetSSock();
			uint32_t smsgId = server->GetMsgId();

			binData.clear();
			binData.resize(msg->ByteSizeLong());
			msg->SerializeToArray(binData.data(), binData.size());
			MessagePack(smsgId, MsgDeal::Req, C2G_ReqAuthAccount::GetDescriptor()->full_name().c_str(), binData);

			{
				// message change to global
				auto taskGen = [](Message* msg) -> DNTask<Message*>
					{
						co_return msg;
					};
				auto dataChannel = taskGen(&response);
				// wait data parse
				server->AddMsg(smsgId, &dataChannel, 9000);
				entity->GetSock()->write(binData);
				co_await dataChannel;
				if (dataChannel.HasFlag(DNTaskFlag::Timeout))
				{
					DNPrint(0, LoggerLevel::Debug, "requst timeout! ");
					response.set_state_code(3);
				}

			}


		}

		binData.clear();
		binData.resize(response.ByteSizeLong());
		response.SerializeToArray(binData.data(), binData.size());

		MessagePack(msgId, MsgDeal::Res, nullptr, binData);

		channel->write(binData);
		co_return;
	}
}
