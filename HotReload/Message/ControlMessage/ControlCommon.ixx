module;
export module ControlMessage:ControlCommon;

import ControlServerHelper;
import ThirdParty.PbGen;
import ThirdParty.Libhv;
import FuncHelper;

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
			response.set_error_code(EL10nCode_RegistServerTypeError);
		}

		//exist?
		else if (ServerEntity::Ptr entity = channel->getContextPtr<ServerEntity>())
		{
			response.set_error_code(EL10nCode_RegistServerChannelExist);
		}

		else if (ServerEntity::Ptr entity = entityMan->AddEntity(request.server_id(), regType))
		{
			size_t pos = ipPort.find(":");
			entity->SetServerIp(ipPort.substr(0, pos));
			entity->SetServerPort(request.server_port());
			entity->SetChannel(channel);

			channel->setContextPtr(entity);

			response.set_ret_server_type(static_cast<uint8_t>(dnServer->GetServerType()));
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
