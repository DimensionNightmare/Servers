module;
export module ControlMessage:ControlRedirect;

import DNTask;
import FuncHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ServerEntity;
import std.compat;
import ServerEntityManagerHelper;
import ControlServerHelper;
import ECSW;

namespace ControlMessage
{
	export DNTaskVoid Msg_ReqAuthAccount(const DNSocketChannel::Ptr& channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::A2g_ReqAuthAccount request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		GMsg::g2A_ResAuthAccount response;

		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
		});

		ServerEntity::Ptr entity = nullptr;

		ControlServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<ControlServerHelper>(EMSystemType::DNServer);

		ServerEntityManagerHelper::Ptr manager = dnServer->GetServerEntityManager();

		const std::list<ServerEntity::Ptr>& serverList = manager->GetEntitysByType(EMServerType::GlobalServer);

		// std::erase_if(serverList, [](ServerEntity::Ptr itor){return itor ? itor->TimerId() : true; });
		// serverList.sort([](ServerEntity::Ptr lhs, ServerEntity::Ptr rhs){return lhs->ConnNum() < rhs->ConnNum(); });

		for (const ServerEntity::Ptr& server : serverList)
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

		if (!entity)
		{
			response.set_state_code(2);
		}
		else
		{

			// message change to global
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);
			// wait data parse

			DNServerProxyHelper::Ptr proxy = dnServer->GetServerProxy();

			uint32_t msgId = proxy->GetMsgId();
			proxy->AddMsg(msgId, &dataChannel, 9000);

			MessagePackAndSend(msgId, EMMsgDeal::Redir, request.GetDescriptor()->full_name(), binMsg, entity->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				dnServer->GetLogger()->Record(ELogLevel_Debug, "requst timeout! ");
				response.set_state_code(3);
			}

		}

		co_return;
	}
}
