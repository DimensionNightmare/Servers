export module ControlServerMessage:ControlRedirect;

import ControlServerHelper;
import ServerEntityHelper;
import FuncHelper;
import Task;
import ThirdParty.PbGen;
import Logger;
import ControlServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::A2g_ReqAuthAccount, GMsg::g2A_ResAuthAccount, EMMsgDeal::Redir> Msg_ReqAuthAccount =
		[](auto request, auto response, SocketChannel::Ptr channel) -> TaskVoid
	{
		
		ServerEntityHelper::Ptr serverEntity = nullptr;

		ControlServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<ControlServerHelper>(EMSystemType::Server);

		ServerEntityManagerHelper::Ptr manager = dnServer->GetServerEntityManager();

		const std::list<ServerEntity::Ptr>& serverList = manager->GetEntitysByType(EMServerType::GlobalServer);

		// std::erase_if(serverList, [](ServerEntity::Ptr itor){return itor ? itor->GetTimerId() : true; });
		// serverList.sort([](ServerEntity::Ptr lhs, ServerEntity::Ptr rhs){return lhs->GetConnNum() < rhs->GetConnNum(); });

		for (ServerEntity::Ptr server : serverList)
		{
			ServerEntityHelper::Ptr entityHelper = server->GetSelf<ServerEntityHelper>();

			if (entityHelper->GetTimerId())
			{
				continue;
			}

			if (!serverEntity)
			{
				serverEntity = entityHelper;
				continue;
			}

			if (entityHelper->GetConnNum() < serverEntity->GetConnNum())
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
			ServerProxyHelper::Ptr proxyHelper = dnServer->GetServerProxy();

			bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Redir, request, response, serverEntity->GetChannel());

			if (!success)
			{
				response->set_errorcode(EL10nCode_SControlReqTimeout);
			}

		}

		co_return;
	};

}
