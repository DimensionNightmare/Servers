export module GateServerMessage:GateClient;

import GateServerHelper;

import StrUtils;
import FuncHelper;
import Task;
import ProxyEntityHelper;
import ThirdParty.PbGen;
import Logger;
import GateServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::C2S_ReqAuthToken, GMsg::S2C_ResAuthToken, EMMsgDeal::Req> Msg_ReqAuthToken =
				[](auto request, auto response, SocketChannel::Ptr channel) -> TaskVoid
	{
		
		GateServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::Server);
		ProxyEntityManagerHelper::Ptr entityMan = dnServer->GetProxyEntityManager();


		ProxyEntityHelper::Ptr entity = entityMan->GetEntity(request->accountid());
		if (!entity)
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "noaccount {}!!", request->accountid());
			response->set_errorcode(EL10nCode_NoneProxyEntity);
		}
		// if not match, timer will destory entity
		else if (Md5Hash(entity->GetToken()) != request->token())
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "not match!!");
			response->set_errorcode(EL10nCode_LoginTokenNotMatch);
		}
		else
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "match!!");

			if (size_t timerId = entity->GetTimerId())
			{
				entity->SetTimerId(0);
				entityMan->GetTimer()->KillTimer(timerId);
			}
			
			channel->setContextPtr(entity);
			entity->SetChannel(channel);


			//DS Server
			ServerEntityManagerHelper::Ptr serverEntityMan = dnServer->GetServerEntityManager();
			ServerEntityHelper::Ptr serverEntity = nullptr;

			// <cache> server to load login data
			if (size_t serverId = entity->GetRecordServerId())
			{
				serverEntity = serverEntityMan->GetEntity(serverId);
			}

			// pool
			if (!serverEntity)
			{
				std::list<ServerEntity::Ptr> serverEntityList = serverEntityMan->GetEntitysByType(EMServerType::LogicServer);
				if (serverEntityList.empty())
				{
					LoggerPrint::Log(channel, ELogLevel_Debug, "Msg_ReqAuthToken not LogicServer !!");
					response->set_errorcode(EL10nCode_NotExistLogicServer);
				}
				else
				{
					serverEntity = serverEntityList.front()->GetSelf<ServerEntityHelper>();
				}
			}

			//req dedicatedServer Info to Login ds.
			if (serverEntity)
			{
				entity->SetRecordServerId(serverEntity->ID());

				ServerProxyHelper::Ptr proxyHelper = dnServer->GetServerProxy();

				bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Redir, request, serverEntity->GetChannel(), response);

				if (!success)
				{
					response->set_errorcode(EL10nCode_SGateReqTimeout);
				}

			}

		}

		co_return;
	};

}
