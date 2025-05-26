module;
export module GateMessage:GateRedirect;

import FuncHelper;
import Logger;
import DNTask;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ServerEntityManagerHelper;
import std.compat;
import GateServerHelper;
import ECSW;

namespace GateMessage
{

	export DNTaskVoid Exe_ReqLoadData(const DNSocketChannel::Ptr& channel, uint32_t msgId, const std::string& binMsg)
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

		GateServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::DNServer);
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();
		const std::list<ServerEntity::Ptr>& dbServers = entityMan->GetEntitysByType(EMServerType::DatabaseServer);

		std::string binData;
		if (dbServers.empty())
		{
			response.set_error_code(EL10nCode_NotExistDBServer);
		}
		else
		{
			ServerEntity::Ptr entity = dbServers.front();

			// data alloc
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			DNServerProxyHelper::Ptr serverProxy = dnServer->GetServerProxy();
			uint32_t msgId = serverProxy->GetMsgId();
			serverProxy->AddMsg(msgId, &dataChannel, 8000);

			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binData, entity->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				response.set_error_code(EL10nCode_SGateReqTimeout);
			}
			
		}

		co_return;
	}

	export DNTaskVoid Exe_ReqSaveData(const DNSocketChannel::Ptr& channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::L2D_ReqSaveData request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		GMsg::D2L_ResSaveData response;

		GateServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::DNServer);
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();
		const std::list<ServerEntity::Ptr>& dbServers = entityMan->GetEntitysByType(EMServerType::DatabaseServer);

		std::string binData;
		if (dbServers.empty())
		{
			response.set_error_code(EL10nCode_NotExistDBServer);
		}
		else
		{
			ServerEntity::Ptr entity = dbServers.front();

			// pack data
			binData = binMsg;

			// data alloc
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			DNServerProxyHelper::Ptr server = dnServer->GetServerProxy();
			uint32_t msgId = server->GetMsgId();
			server->AddMsg(msgId, &dataChannel, 8000);

			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binData, entity->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				dnServer->GetLogger()->Record(ELogLevel_Debug, "requst timeout! ");
				response.set_error_code(EL10nCode_SGateReqTimeout);
			}
			
		}

		response.SerializeToString(&binData);
		MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);

		co_return;
	}
}
