module;
export module ControlMessage:ControlCommon;

import DNTask;
import FuncHelper;
import ControlServerHelper;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import Logger;
import ServerEntityManagerHelper;
import std.compat;

namespace ControlMessage
{

	// client request
	export void Msg_ReqRegistSrv(hv::SocketChannelPtr channel, uint32_t msgId, std::string binMsg)
	{
		GMsg::COM_ReqRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GMsg::COM_ResRegistSrv response;

		ControlServerHelper* dnServer = GetControlServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();

		EMServerType regType = (EMServerType)request.server_type();

		LoggerPrint()(ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request.server_type());

		const std::string& ipPort = channel->localaddr();

		if (regType < EMServerType::GlobalServer || regType > EMServerType::AuthServer || ipPort.empty())
		{
			response.set_success(false);
		}

		//exist?
		else if (ServerEntity* entity = channel->getContext<ServerEntity>())
		{
			response.set_success(false);
		}

		else if (ServerEntity* entity = entityMan->AddEntity(entityMan->GenServerId(), regType))
		{
			size_t pos = ipPort.find(":");
			entity->ServerIp() = ipPort.substr(0, pos);
			entity->ServerPort() = request.server_port();
			entity->SetSock(channel);

			channel->setContext(entity);

			response.set_success(true);
			response.set_server_id(entity->ID());
			response.set_server_type((uint8_t(dnServer->GetServerType())));
		}

		std::string binData;
		response.SerializeToString(&binData);

		MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);
	}

	export void Exe_RetHeartbeat(hv::SocketChannelPtr channel, std::string binMsg)
	{
		GMsg::COM_RetHeartbeat request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
	}
}
