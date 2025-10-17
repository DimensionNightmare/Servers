export module ControlServerMessage:ControlCommon;

import ControlServerHelper;

import ThirdParty.Libhv;
import FuncHelper;
import ServerEntityHelper;
import std;
import ThirdParty.PbGen;
import Logger;

export namespace ControlServerMessage
{

	// client request
	void Msg_ReqRegistSrv(SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
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

		ControlServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<ControlServerHelper>(EMSystemType::Server);

		ServerEntityManagerHelper::CVPtr entityMan = dnServer->GetServerEntityManager();

		LoggerPrint::Log(channel, ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request.servertype());

		const std::string& ipPort = channel->localaddr();

		EMServerType regType = (EMServerType)request.servertype();

		if (regType < EMServerType::GlobalServer || regType > EMServerType::AuthServer || ipPort.empty())
		{
			response.set_errorcode(EL10nCode_RegistServerTypeError);
		}

		//exist?
		else if (ServerEntityHelper::Ptr entity = channel->getContextPtr<ServerEntityHelper>())
		{
			response.set_errorcode(EL10nCode_RegistServerChannelExist);
		}

		else if (entity = entityMan->AddEntity(request.serverid(), regType))
		{
			size_t pos = ipPort.find(":");
			entity->SetServerIp(ipPort.substr(0, pos));
			entity->SetServerPort(request.serverport());
			entity->SetChannel(channel);

			channel->setContextPtr(entity);

			response.set_retservertype(static_cast<uint8_t>(dnServer->GetServerType()));
		}

		
	}

	void Exe_RetHeartbeat(SocketChannel::CVPtr channel, const std::string& binMsg)
	{
		GMsg::COM_RetHeartbeat request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
	}

}
