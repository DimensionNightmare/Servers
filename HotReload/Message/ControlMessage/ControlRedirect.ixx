module;
#include <coroutine>
#include <cstdint>
#include <list>
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Server/S_Auth.pb.h"
export module ControlMessage:ControlRedirect;

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
		g2A_ResAuthAccount response;

		ServerEntity* entity = nullptr;
		const list<ServerEntity*>& serverList = GetControlServer()->GetServerEntityManager()->GetEntityByList(ServerType::GlobalServer);

		// std::erase_if(serverList, [](ServerEntity* itor){return itor ? itor->TimerId() : true; });
		// serverList.sort([](ServerEntity* lhs, ServerEntity* rhs){return lhs->ConnNum() < rhs->ConnNum(); });

		for (ServerEntity* server : serverList)
		{
			if (server->TimerId())
			{
				continue;
			}

			if (!entity)
			{
				entity = server;
				continue;
			}

			if (server->ConnNum() < entity->ConnNum())
			{
				entity = server;
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
			uint32_t msgId = server->GetMsgId();

			msg->SerializeToString(&binData);
			MessagePack(msgId, MsgDeal::Redir, msg->GetDescriptor()->full_name().c_str(), binData);

			{
				// message change to global
				auto taskGen = [](Message* msg) -> DNTask<Message*>
					{
						co_return msg;
					};
				auto dataChannel = taskGen(&response);
				// wait data parse
				server->AddMsg(msgId, &dataChannel, 9000);
				entity->GetSock()->write(binData);
				co_await dataChannel;
				if (dataChannel.HasFlag(DNTaskFlag::Timeout))
				{
					DNPrint(0, LoggerLevel::Debug, "requst timeout! ");
					response.set_state_code(3);
				}

			}


		}

		response.SerializeToString(&binData);

		MessagePack(msgId, MsgDeal::Res, nullptr, binData);

		channel->write(binData);
		co_return;
	}
}
