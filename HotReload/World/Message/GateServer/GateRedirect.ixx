export module GateServerMessage:GateRedirect;

import GateServerHelper;
import FuncHelper;
import Task;
import ThirdParty.PbGen;
import Logger;
import GateServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::L2D_ReqLoadData, GMsg::D2L_ResLoadData, EMMsgDeal::Redir> Exe_ReqLoadData =
				[](auto request, auto response, SocketChannel::Ptr channel) -> TaskVoid
	{
		
		GateServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();
		const std::list<ServerEntity::Ptr>& dbServers = entityMan->GetEntitysByType(EMServerType::DatabaseServer);

		if (dbServers.empty())
		{
			response->set_errorcode(EL10nCode_NotExistDBServer);
		}
		else
		{
			ServerEntityHelper::Ptr entity = dbServers.front()->GetSelf<ServerEntityHelper>();

		
			ServerProxyHelper::Ptr proxyHelper = dnServer->GetServerProxy();
			
			bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Req, request, response, entity->GetChannel());

			if (!success)
			{
				response->set_errorcode(EL10nCode_SGateReqTimeout);
			}
			
		}

		co_return;
	};

	HandleRegistry<GMsg::L2D_ReqSaveData, GMsg::D2L_ResSaveData, EMMsgDeal::Redir> Exe_ReqSaveData =
				[](auto request, auto response, SocketChannel::Ptr channel) -> TaskVoid
	{
		
		GateServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();
		const std::list<ServerEntity::Ptr>& dbServers = entityMan->GetEntitysByType(EMServerType::DatabaseServer);

		if (dbServers.empty())
		{
			response->set_errorcode(EL10nCode_NotExistDBServer);
		}
		else
		{
			ServerEntityHelper::Ptr entity = dbServers.front()->GetSelf<ServerEntityHelper>();

			ServerProxyHelper::Ptr proxyHelper = dnServer->GetServerProxy();
	
			bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Req, request, response, entity->GetChannel());

			if (!success)
			{
				LoggerPrint::Log(channel, ELogLevel_Debug, "requst timeout! ");
				response->set_errorcode(EL10nCode_SGateReqTimeout);
			}
			
		}

		co_return;
	};
}
