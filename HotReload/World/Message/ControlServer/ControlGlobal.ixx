export module ControlServerMessage:ControlGlobal;

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
		[](World::Ptr world, SocketChannel::Ptr channel, auto request, auto response) -> TaskVoid
	{
		
		ControlServerHelper::CVPtr dnServer = world->GetSystem<ControlServerHelper>(EMSystemType::Server);

		ServerEntityManagerHelper::CVPtr entityMan = dnServer->GetServerEntityManager();

		auto selects = entityMan->GetEntitysByType(EMServerType::GlobalServer)
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
			ServerProxyHelper::CVPtr proxyHelper = dnServer->GetServerProxy();

			bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Redir, request, (*it)->GetChannel(), response);

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

	HandleRegistry<GMsg::A2g_ReqLogicServerIp, GMsg::g2A_ResLogicServerIp, EMMsgDeal::Redir> Msg_ReqLogicServerIp =
		[](World::Ptr world, SocketChannel::Ptr channel, auto request, auto response) -> TaskVoid
	{
		
		ControlServerHelper::CVPtr dnServer = world->GetSystem<ControlServerHelper>(EMSystemType::Server);

		ServerEntityManagerHelper::CVPtr entityMan = dnServer->GetServerEntityManager();

		auto selects = entityMan->GetEntitysByType(EMServerType::GlobalServer)
			| std::views::transform([](const auto& param){
				return param->GetSelf<ServerEntityHelper>();
			})
			| std::views::filter([](const auto& param){
				return param->GetTimerId() == 0;
			});


		if (auto it = std::ranges::min_element(selects, {}, &ServerEntityHelper::GetConnNum); it != selects.end())
		{
			ServerProxyHelper::CVPtr proxyHelper = dnServer->GetServerProxy();
			
			bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Redir, request, (*it)->GetChannel(), response);
			
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
