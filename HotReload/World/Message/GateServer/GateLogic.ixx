export module GateServerMessage:GateLogic;

import GateServerMessage;
import GateServerHelper;
import FuncHelper;
import Task;
import ThirdParty.PbGen;
import Logger;
import ProxyEntityHelper;

namespace MsgHandleRegister
{
	HandleRegistry<GMsg::S2C_ReqGetRoom, GMsg::S2C_ResGetRoom, EMMsgDeal::Req> S2C_ReqGetRoom =
				[](World::Ptr world, SocketChannel::Ptr channel, auto request, auto response) -> TaskVoid
	{
		ProxyEntityHelper::CVPtr entity = channel->GetEntity<ProxyEntityHelper>();
		if (!entity)
		{
			LoggerPrint::Log(world, ELogLevel_Debug, "noaccount {}!!", request->accountid());
			response->set_errorcode(EL10nCode_NoneProxyEntity);
			co_return;
		}

		request->set_accountid(entity->ID());

		
		GateServerHelper::CVPtr dnServer = world->GetSystem<GateServerHelper>(EMSystemType::Server);
		
		ServerEntityManagerHelper::CVPtr entityMan = dnServer->GetServerEntityManager();
		
		//DS Server
		ServerEntityHelper::Ptr serverEntity;

		// <cache> server to load login data
		if (size_t serverId = entity->GetRecordServerId())
		{
			serverEntity = entityMan->GetEntity(serverId);
		}

		// pool
		if (!serverEntity)
		{
			auto selects = entityMan->GetEntitysByType(EMServerType::LogicServer)
			| std::views::transform([](const auto& param){
				return param->GetSelf<ServerEntityHelper>();
			})
			| std::views::filter([](const auto& param)
				{
					return param->GetTimerId() == 0;
				})
			;

			if(auto it = std::ranges::min_element(selects, std::greater{}, &ServerEntityHelper::GetConnNum); it != selects.end())
			{
				serverEntity = *it;
			}
			else
			{
				LoggerPrint::Log(world, ELogLevel_Debug, "Msg_ReqAuthToken not LogicServer !!");
				response->set_errorcode(EL10nCode_NotExistLogicServer);
			}
		}

		//req dedicatedServer Info to Login ds.
		if (serverEntity)
		{
			entity->SetRecordServerId(serverEntity->ID());

			ServerProxyHelper::CVPtr proxyHelper = dnServer->GetServerProxy();

			bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Redir, request, serverEntity->GetChannel(), response);

			if (!success)
			{
				response->set_errorcode(EL10nCode_SGateReqTimeout);
			}

		}
		co_return;
	};
}