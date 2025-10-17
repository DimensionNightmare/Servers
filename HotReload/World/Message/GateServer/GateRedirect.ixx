export module GateServerMessage:GateRedirect;

import GateServerHelper;

import ThirdParty.Libhv;
import FuncHelper;
import Task;
import std;
import ThirdParty.PbGen;
import Logger;
import ThirdParty.Protobuf;

export namespace GateServerMessage
{

	TaskVoid Exe_ReqLoadData(SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::L2D_ReqLoadData request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		GMsg::D2L_ResLoadData response;

		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
		});

		GateServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();
		const std::list<ServerEntity::Ptr>& dbServers = entityMan->GetEntitysByType(EMServerType::DatabaseServer);

		if (dbServers.empty())
		{
			response.set_errorcode(EL10nCode_NotExistDBServer);
		}
		else
		{
			ServerEntityHelper::Ptr entity = dbServers.front()->GetSelf<ServerEntityHelper>();

			// data alloc
			auto taskGen = [](Message* msg) -> Task<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			ServerProxyHelper::Ptr serverProxy = dnServer->GetServerProxy();
			uint32_t msgId = serverProxy->GetMsgId();
			serverProxy->AddMsg(msgId, &dataChannel, 8000);

			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binMsg, entity->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMTaskFlag::Timeout))
			{
				response.set_errorcode(EL10nCode_SGateReqTimeout);
			}
			
		}

		co_return;
	}

	TaskVoid Exe_ReqSaveData(SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::L2D_ReqSaveData request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		GMsg::D2L_ResSaveData response;
		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
		});

		GateServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();
		const std::list<ServerEntity::Ptr>& dbServers = entityMan->GetEntitysByType(EMServerType::DatabaseServer);

		if (dbServers.empty())
		{
			response.set_errorcode(EL10nCode_NotExistDBServer);
		}
		else
		{
			ServerEntityHelper::Ptr entity = dbServers.front()->GetSelf<ServerEntityHelper>();

			// data alloc
			auto taskGen = [](Message* msg) -> Task<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			ServerProxyHelper::Ptr server = dnServer->GetServerProxy();
			uint32_t msgId = server->GetMsgId();
			server->AddMsg(msgId, &dataChannel, 8000);

			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binMsg, entity->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMTaskFlag::Timeout))
			{
				LoggerPrint::Log(channel, ELogLevel_Debug, "requst timeout! ");
				response.set_errorcode(EL10nCode_SGateReqTimeout);
			}
			
		}

		co_return;
	}

}
