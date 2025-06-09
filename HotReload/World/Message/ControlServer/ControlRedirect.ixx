module;
export module ControlServerMessage:ControlRedirect;

import ServerEntity;
import ControlServerHelper;
import ServerEntityHelper;
import ThirdParty.Libhv;
import FuncHelper;
import DNTask;

namespace ControlServerMessage
{
	export DNTaskVoid Msg_ReqAuthAccount(DNSocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
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

		ServerEntityHelper::Ptr serverEntity = nullptr;

		ControlServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<ControlServerHelper>(EMSystemType::DNServer);

		ServerEntityManagerHelper::CVPtr manager = dnServer->GetServerEntityManager();

		const std::list<ServerEntity::Ptr>& serverList = manager->GetEntitysByType(EMServerType::GlobalServer);

		// std::erase_if(serverList, [](ServerEntity::Ptr itor){return itor ? itor->TimerId() : true; });
		// serverList.sort([](ServerEntity::Ptr lhs, ServerEntity::Ptr rhs){return lhs->ConnNum() < rhs->ConnNum(); });

		for (ServerEntity::CVPtr server : serverList)
		{
			ServerEntityHelper::CVPtr entityHelper = server->GetSelf<ServerEntityHelper>();

			if (entityHelper->TimerId())
			{
				continue;
			}

			if (!serverEntity)
			{
				serverEntity = entityHelper;
				continue;
			}

			if (entityHelper->ConnNum() < serverEntity->ConnNum())
			{
				serverEntity = entityHelper;
			}
		}

		if (!serverEntity)
		{
			response.set_error_code(EL10nCode_NotExistGlobalServer);
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

			DNServerProxyHelper::CVPtr proxy = dnServer->GetServerProxy();

			uint32_t msgId = proxy->GetMsgId();
			proxy->AddMsg(msgId, &dataChannel, 9000);

			MessagePackAndSend(msgId, EMMsgDeal::Redir, request.GetDescriptor()->full_name(), binMsg, serverEntity->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				response.set_error_code(EL10nCode_SControlReqTimeout);
			}

		}

		co_return;
	}
}
