export module GateServerMessage:GateDatabase;

import GateServerHelper;
import FuncHelper;
import Task;
import ThirdParty.PbGen;
import Logger;
import GateServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::L2D_ReqLoadData, GMsg::D2L_ResLoadData, EMMsgDeal::Redir> Exe_ReqLoadData =
				[](World::Ptr world, SocketChannel::Ptr channel, auto request, auto response) -> TaskVoid
	{
		
		GateServerHelper::CVPtr dnServer = world->GetSystem<GateServerHelper>(EMSystemType::Server);

		ServerEntityManagerHelper::CVPtr entityMan = dnServer->GetServerEntityManager();

		auto selects = entityMan->GetEntitysByType(EMServerType::DatabaseServer)
			| std::views::transform([](const auto& param){
				return param->GetSelf<ServerEntityHelper>();
			})
			| std::views::filter([](const auto& param)
				{
					return param->GetTimerId() == 0;
				})
			;

		if (selects.empty())
		{
			response->set_errorcode(EL10nCode_NotExistDBServer);
		}
		else if(auto it = std::ranges::min_element(selects, std::greater{}, &ServerEntityHelper::GetConnNum); it != selects.end())
		{
			ServerEntityHelper::CVPtr entity = *it;

			ServerProxyHelper::CVPtr proxyHelper = dnServer->GetServerProxy();
			
			bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Req, request, entity->GetChannel(), response);

			if (!success)
			{
				response->set_errorcode(EL10nCode_SGateReqTimeout);
			}
			
		}

		co_return;
	};

	HandleRegistry<GMsg::L2D_ReqSaveData, GMsg::D2L_ResSaveData, EMMsgDeal::Redir> Exe_ReqSaveData =
				[](World::Ptr world, SocketChannel::Ptr channel, auto request, auto response) -> TaskVoid
	{
		
		GateServerHelper::CVPtr dnServer = world->GetSystem<GateServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::CVPtr entityMan = dnServer->GetServerEntityManager();

		auto selects = entityMan->GetEntitysByType(EMServerType::DatabaseServer)
			| std::views::transform([](const auto& param){
				return param->GetSelf<ServerEntityHelper>();
			})
			| std::views::filter([](const auto& param)
				{
					return param->GetTimerId() == 0;
				})
			;

		if (selects.empty())
		{
			response->set_errorcode(EL10nCode_NotExistDBServer);
		}
		else if(auto it = std::ranges::min_element(selects, std::greater{}, &ServerEntityHelper::GetConnNum); it != selects.end())
		{
			ServerEntityHelper::CVPtr entity = *it;

			ServerProxyHelper::CVPtr proxyHelper = dnServer->GetServerProxy();
	
			bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Req, request, entity->GetChannel(), response);

			if (!success)
			{
				LoggerPrint::Log(world, ELogLevel_Debug, "requst timeout! ");
				response->set_errorcode(EL10nCode_SGateReqTimeout);
			}
			
		}

		co_return;
	};
}
