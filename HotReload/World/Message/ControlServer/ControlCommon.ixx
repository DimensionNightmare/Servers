export module ControlServerMessage:ControlCommon;

import ControlServerHelper;
import FuncHelper;
import ServerEntityHelper;
import ThirdParty.PbGen;
import Logger;
import ControlServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::COM_ReqRegistSrv, GMsg::COM_ResRegistSrv, EMMsgDeal::Req> Msg_ReqRegistSrv =
		[](World::Ptr world, SocketChannel::Ptr channel, auto request, auto response)
	{
		
		ControlServerHelper::CVPtr dnServer = world->GetSystem<ControlServerHelper>(EMSystemType::Server);

		ServerEntityManagerHelper::CVPtr entityMan = dnServer->GetServerEntityManager();

		LoggerPrint::Log(world, ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request->servertype());

		const std::string& ipPort = channel->localaddr();

		EMServerType regType = static_cast<EMServerType>(request->servertype());

		ServerEntityHelper::Ptr entity;

		if (regType < EMServerType::GlobalServer || regType > EMServerType::AuthServer || ipPort.empty())
		{
			response->set_errorcode(EL10nCode_RegistServerTypeError);
		}

		//exist?
		else if (entity = channel->GetEntity<ServerEntityHelper>())
		{
			response->set_errorcode(EL10nCode_RegistServerChannelExist);
		}

		else if (entity = entityMan->AddEntity(request->serverid(), regType))
		{
			size_t pos = ipPort.find(":");
			entity->SetServerIp(ipPort.substr(0, pos));
			entity->SetServerPort(request->serverport());
			entity->SetChannel(channel);

			channel->SetEntity(entity);

			response->set_retservertype(std::to_underlying(dnServer->GetServerType()));
		}
	};

	HandleRegistry<GMsg::COM_RetHeartbeat, void, EMMsgDeal::Ret> Exe_RetHeartbeat =
		[](World::Ptr world, SocketChannel::Ptr channel, auto request)
	{
		
	};

}
