export module ControlServerMessage:ControlRedirect;

import ServerEntity;
import ControlServerHelper;
import ServerEntityHelper;
import ThirdParty.Libhv;
import FuncHelper;
import Task;
import std;
import ThirdParty.PbGen;
import Logger;
import ThirdParty.Protobuf;
import ControlServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::A2g_ReqAuthAccount, GMsg::g2A_ResAuthAccount, EMMsgDeal::Redir> Msg_ReqAuthAccount(
		[](auto request, auto response, SocketChannel::Ptr channel) -> TaskVoid
	{
		
		ServerEntityHelper::Ptr serverEntity = nullptr;

		ControlServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<ControlServerHelper>(EMSystemType::Server);

		ServerEntityManagerHelper::Ptr manager = dnServer->GetServerEntityManager();

		const std::list<ServerEntity::Ptr>& serverList = manager->GetEntitysByType(EMServerType::GlobalServer);

		// std::erase_if(serverList, [](ServerEntity::Ptr itor){return itor ? itor->TimerId() : true; });
		// serverList.sort([](ServerEntity::Ptr lhs, ServerEntity::Ptr rhs){return lhs->ConnNum() < rhs->ConnNum(); });

		for (ServerEntity::Ptr server : serverList)
		{
			ServerEntityHelper::Ptr entityHelper = server->GetSelf<ServerEntityHelper>();

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
			response->set_errorcode(EL10nCode_NotExistGlobalServer);
		}
		else
		{

			// message change to global
			auto taskGen = [](Message* msg) -> Task<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(response);
			// wait data parse

			ServerProxyHelper::Ptr proxy = dnServer->GetServerProxy();

			uint32_t msgId = proxy->GetMsgId();
			proxy->AddMsg(msgId, &dataChannel, 9000);

			std::string binMsg;
			request->SerializeToString(&binMsg);
			MessagePackAndSend(msgId, EMMsgDeal::Redir, request->GetDescriptor()->full_name(), binMsg, serverEntity->GetChannel());

			{
				co_await dataChannel;
				if (dataChannel.HasFlag(EMTaskFlag::Timeout))
				{
					response->set_errorcode(EL10nCode_SControlReqTimeout);
				}
			}

		}

		co_return;
	});

}
