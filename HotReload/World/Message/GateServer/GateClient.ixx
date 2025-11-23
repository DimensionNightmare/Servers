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
				[](World::Ptr world, SocketChannel::Ptr channel, auto request, auto response) -> TaskVoid
	{
		
		GateServerHelper::CVPtr dnServer = world->GetSystem<GateServerHelper>(EMSystemType::Server);
		ProxyEntityManagerHelper::CVPtr entityMan = dnServer->GetProxyEntityManager();


		ProxyEntityHelper::CVPtr entity = entityMan->GetEntity(request->accountid());
		if (!entity)
		{
			LoggerPrint::Log(world, ELogLevel_Debug, "noaccount {}!!", request->accountid());
			response->set_errorcode(EL10nCode_NoneProxyEntity);
		}
		// if not match, timer will destory entity
		else if (Md5Hash(entity->GetToken()) != request->token())
		{
			LoggerPrint::Log(world, ELogLevel_Debug, "not match!!");
			response->set_errorcode(EL10nCode_LoginTokenNotMatch);
		}
		else
		{
			LoggerPrint::Log(world, ELogLevel_Debug, "match!!");

			if (size_t timerId = entity->GetTimerId())
			{
				entity->SetTimerId(0);
				entityMan->GetTimer()->KillTimer(timerId);
			}
			
			channel->SetEntity(entity);
			entity->SetChannel(channel);

		}

		co_return;
	};

}
