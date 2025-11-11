export module GlobalServerMessage:GlobalRedirect;

import FuncHelper;
import GlobalServerHelper;
import ThirdParty.Libhv;
import Task;
import ServerEntity;
import ThirdParty.PbGen;
import Logger;
import GlobalServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::A2g_ReqAuthAccount, GMsg::g2A_ResAuthAccount, EMMsgDeal::Redir> Msg_ReqAuthAccount =
				[](auto request, auto response, SocketChannel::Ptr channel) -> TaskVoid
	{
		// if has db not need origin
		GlobalServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GlobalServerHelper>(EMSystemType::Server);
		std::list<ServerEntity::Ptr> serverList = dnServer->GetServerEntityManager()->GetEntitysByType(EMServerType::GateServer);

		auto selects = serverList 
			| std::ranges::views::transform([](const auto& server){
				return server->GetSelf<ServerEntityHelper>();
			})
			| std::views::filter([](const auto& server){
				return server->HasFlag(EMServerEntityFlag::Locked) == true;
			})
			;

		if ( auto it = std::ranges::min_element(selects, std::greater{}, &ServerEntityHelper::GetConnNum); it != selects.end())
		{
			ServerEntityHelper::Ptr entity = *it;
			LoggerPrint::Log(channel, ELogLevel_Debug, "send to GateServer : {}", entity->ID());

			entity->SetConnNum(1);

			ServerProxyHelper::Ptr proxyHelper = dnServer->GetServerProxy();

			bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Req, request, response, entity->GetChannel());
			
			if (!success)
			{
				response->set_errorcode(EL10nCode_SGlobalReqTimeout);

			}

			if(response->errorcode() == EL10nCode_None)
			{
				response->set_serverip(entity->GetServerIp());
				response->set_serverport(entity->GetServerPort());
			}
			else
			{
				entity->SetConnNum(-1);
			}

			LoggerPrint::Log(channel, ELogLevel_Debug, "Msg_ReqAuthAccount:{}", response->DebugString());
		}
		else
		{
			response->set_errorcode(EL10nCode_NotExistGateServer);
		}

		co_return;
	};

	HandleRegistry<GMsg::A2g_ReqLogicServerIp, GMsg::g2A_ResLogicServerIp, EMMsgDeal::Redir> Msg_ReqLogicServerIp =
		[](auto request, auto response, SocketChannel::Ptr channel) -> TaskVoid
	{
		
		// if has db not need origin
		GlobalServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GlobalServerHelper>(EMSystemType::Server);
		std::list<ServerEntity::Ptr> serverList = dnServer->GetServerEntityManager()->GetEntitysByType(EMServerType::GateServer);

		std::list<ServerEntityHelper::Ptr> tempList;
		for (ServerEntity::Ptr& server : serverList)
		{
			if (server->HasFlag(EMServerEntityFlag::Locked))
			{
				tempList.emplace_back(server->GetSelf<ServerEntityHelper>());
			}
		}

		tempList.sort([](ServerEntityHelper::Ptr lhs, ServerEntityHelper::Ptr rhs) { return lhs->GetConnNum() < rhs->GetConnNum(); });

		if (tempList.empty())
		{
			response->set_errorcode(EL10nCode_NotExistGateServer);
		}
		else
		{
			ServerEntityHelper::Ptr entity = tempList.front();
			LoggerPrint::Log(channel, ELogLevel_Debug, "send to GateServer : {}", entity->ID());

			ServerProxyHelper::Ptr proxyHelper = dnServer->GetServerProxy();

			bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Req, request, response, entity->GetChannel());
			
			if (!success)
			{
				response->set_errorcode(EL10nCode_SGlobalReqTimeout);

			}

			LoggerPrint::Log(channel, ELogLevel_Debug, "Msg_ReqLogicServerIp:{}", response->DebugString());
		}

		co_return;
	};
}