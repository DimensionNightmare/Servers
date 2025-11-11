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

		auto selects = serverList
			| std::views::transform([](const auto& server)
				{
					return server->GetSelf<ServerEntityHelper>();
				})
			| std::views::filter([](const auto& server)
				{
					return server->GetTimerId() == 0;
				});

		if (auto it = std::ranges::min_element(selects, std::greater{}, &ServerEntityHelper::GetConnNum); it != selects.end())
		{
			ServerProxyHelper::Ptr proxyHelper = dnServer->GetServerProxy();

			bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Redir, request, response, (*it)->GetChannel());

			if (!success)
			{
				response->set_errorcode(EL10nCode_SControlReqTimeout);
			}
		}
		else
		{
			response->set_errorcode(EL10nCode_NotExistGlobalServer);
		}
	

		co_return;
	};

}
