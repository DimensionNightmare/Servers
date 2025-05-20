module;
export module ControlMessage:ControlCommon;

import DNTask;
import FuncHelper;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import Logger;
import ServerEntityManagerHelper;
import std.compat;
import DNServer;
import ControlServerHelper;
import ECSW;

namespace ControlMessage
{

	// client request
	export void Msg_ReqRegistSrv(const DNSocketChannel::Ptr& channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::COM_ReqRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GMsg::COM_ResRegistSrv response;

		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
		});

		ControlServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<ControlServerHelper>(EMSystemType::DNServer);

		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();

		dnServer->GetLogger()->Record(ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request.server_type());

		const std::string& ipPort = channel->localaddr();

		EMServerType regType = (EMServerType)request.server_type();

		if (regType < EMServerType::GlobalServer || regType > EMServerType::AuthServer || ipPort.empty())
		{
			response.set_success(false);
		}

		//exist?
		else if (ServerEntity::Ptr entity = channel->getContextPtr<ServerEntity>())
		{
			response.set_success(false);
		}

		else if (ServerEntity::Ptr entity = entityMan->AddEntity(entityMan->GenServerId(), regType))
		{
			size_t pos = ipPort.find(":");
			entity->SetServerIp(ipPort.substr(0, pos));
			entity->SetServerPort(request.server_port());
			entity->SetChannel(channel);

			channel->setContextPtr(entity);

			response.set_success(true);
			response.set_server_id(entity->ID());
			response.set_server_type(static_cast<uint8_t>(dnServer->GetServerType()));
		}

		
	}

	export void Exe_RetHeartbeat(const DNSocketChannel::Ptr& channel, const std::string& binMsg)
	{
		GMsg::COM_RetHeartbeat request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
	}
}
