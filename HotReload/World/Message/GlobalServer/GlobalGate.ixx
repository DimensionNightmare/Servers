export module GlobalServerMessage:GlobalGate;

import GlobalServerHelper;
import ThirdParty.PbGen;
import Logger;
import GlobalServerMessage;
import FuncHelper;
import ThirdParty.Libhv;
import Task;
import ServerEntity;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::g2G_RetRegistSrv, void, EMMsgDeal::Ret> Exe_RetRegistSrv =
				[](World::Ptr world, SocketChannel::Ptr channel, auto request)
	{
		
		GlobalServerHelper::CVPtr dnServer = world->GetSystem<GlobalServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::CVPtr entityMan = dnServer->GetServerEntityManager();
		if (ServerEntityHelper::CVPtr entity = entityMan->GetEntity(request->serverid()))
		{
			if (request->isregist())
			{
				if (size_t timerId = entity->GetTimerId())
				{
					entity->SetTimerId(0);
					entityMan->GetTimer()->KillTimer(timerId);
				}
			}
			else
			{
				ServerEntity::CVPtr owner = channel->GetEntity<ServerEntity>();
				// remove and unlock
				owner->GetMapLinkNode(entity->GetServerType()).remove(entity);
				owner->ClearFlag(EMServerEntityFlag::Locked);

				LoggerPrint::Log(world, ELogLevel_Debug, "Global get notify release gate lock!");

				entityMan->DisposeEntity(entity);
				dnServer->UpdateServerGroup();
			}
		}
	};

	HandleRegistry<GMsg::g2G_RetRegistChild, void, EMMsgDeal::Ret> Exe_RetRegistChild =
				[](World::Ptr world, SocketChannel::Ptr channel, auto request)
	{
				GlobalServerHelper::CVPtr dnServer = world->GetSystem<GlobalServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::CVPtr entityMan = dnServer->GetServerEntityManager();

		ServerEntityHelper::CVPtr entity = entityMan->GetEntity(request->serverid());
		if(!entity)
		{
			return;
		}

		for (int i = 0; i < request->childs_size(); i++)
		{
			const GMsg::COM_ReqRegistSrv& child = request->childs(i);
			EMServerType childType = static_cast<EMServerType>(child.servertype());
			ServerEntityHelper::CVPtr servChild = entityMan->AddEntity(child.serverid(), childType);
			servChild->SetLinkNode(entity);
			entity->SetMapLinkNode(childType, servChild->GetSelf<ServerEntity>());
		}
	};

	HandleRegistry<GMsg::A2g_ReqAuthAccount, GMsg::g2A_ResAuthAccount, EMMsgDeal::Redir> Msg_ReqAuthAccount =
				[](World::Ptr world, SocketChannel::Ptr channel, auto request, auto response) -> TaskVoid
	{
				// if has db not need origin
		GlobalServerHelper::CVPtr dnServer = world->GetSystem<GlobalServerHelper>(EMSystemType::Server);

		auto selects = dnServer->GetServerEntityManager()->GetEntitysByType(EMServerType::GateServer) 
			| std::views::transform([](const auto& server){
				return server->GetSelf<ServerEntityHelper>();
			})
			| std::views::filter([](const auto& server){
				return server->HasFlag(EMServerEntityFlag::Locked) == true;
			})
			;

		if ( auto it = std::ranges::min_element(selects, std::greater{}, &ServerEntityHelper::GetConnNum); it != selects.end())
		{
			ServerEntityHelper::CVPtr entity = *it;
			LoggerPrint::Log(world, ELogLevel_Debug, "send to GateServer : {}", entity->ID());

			entity->SetConnNum(1);

			bool success = co_await dnServer->GetServerProxy()->AddMsg(EMMsgDeal::Req, request, entity->GetChannel(), response);
			
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

			LoggerPrint::Log(world, ELogLevel_Debug, "Msg_ReqAuthAccount:{}", response->DebugString());
		}
		else
		{
			response->set_errorcode(EL10nCode_NotExistGateServer);
		}

		co_return;
	};

	HandleRegistry<GMsg::A2g_ReqLogicServerIp, GMsg::g2A_ResLogicServerIp, EMMsgDeal::Redir> Msg_ReqLogicServerIp =
		[](World::Ptr world, SocketChannel::Ptr channel, auto request, auto response) -> TaskVoid
	{
		
		// if has db not need origin
		GlobalServerHelper::CVPtr dnServer = world->GetSystem<GlobalServerHelper>(EMSystemType::Server);

		auto selects = dnServer->GetServerEntityManager()->GetEntitysByType(EMServerType::GateServer)
			| std::views::filter([](const auto& param){
				return param->HasFlag(EMServerEntityFlag::Locked) == true;
			})
			| std::views::transform([](const auto& param){
				return param->GetSelf<ServerEntityHelper>();
			})
			;

		if (auto it = std::ranges::min_element(selects, {}, &ServerEntityHelper::GetConnNum); it != selects.end())
		{
			ServerEntityHelper::CVPtr entity = *it;
			LoggerPrint::Log(world, ELogLevel_Debug, "send to GateServer : {}", entity->ID());
			
			ServerProxyHelper::CVPtr proxyHelper = dnServer->GetServerProxy();
			
			bool success = co_await proxyHelper->AddMsg(EMMsgDeal::Req, request, entity->GetChannel(), response);
			
			if (!success)
			{
				response->set_errorcode(EL10nCode_SGlobalReqTimeout);
				
			}
			
			LoggerPrint::Log(world, ELogLevel_Debug, "Msg_ReqLogicServerIp:{}", response->DebugString());
		}
		else
		{
			response->set_errorcode(EL10nCode_NotExistGateServer);
		}

		co_return;
	};


}
