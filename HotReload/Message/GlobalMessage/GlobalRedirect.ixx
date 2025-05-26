module;
export module GlobalMessage:GlobalRedirect;

import FuncHelper;
import GlobalServerHelper;
import ThirdParty.PbGen;
import ThirdParty.Libhv;
import DNTask;

namespace GlobalMessage
{

	export DNTaskVoid Msg_ReqAuthAccount(const DNSocketChannel::Ptr& channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::A2g_ReqAuthAccount request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		GMsg::g2A_ResAuthAccount response;

		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
		});

		// if has db not need origin
		GlobalServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GlobalServerHelper>(EMSystemType::DNServer);
		std::list<ServerEntity::Ptr> serverList = dnServer->GetServerEntityManager()->GetEntitysByType(EMServerType::GateServer);

		std::list<ServerEntity::Ptr> tempList;
		for (ServerEntity::Ptr server : serverList)
		{
			if (server->HasFlag(EMServerEntityFlag::Locked))
			{
				tempList.emplace_back(server);
			}
		}

		tempList.sort([](ServerEntity::Ptr lhs, ServerEntity::Ptr rhs) { return lhs->ConnNum() < rhs->ConnNum(); });


		std::string binData;
		if (tempList.empty())
		{
			response.set_error_code(EL10nCode_NotExistGateServer);
		}
		else
		{
			ServerEntity::Ptr entity = tempList.front();
			dnServer->GetLogger()->Record(ELogLevel_Debug, "send to GateServer : {}", entity->ID());

			entity->SetConnNum(1);

			// pack data
			binData = binMsg;

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
				response.set_error_code(EL10nCode_SGlobalReqTimeout);

			}

			if(response.error_code() == EL10nCode_None)
			{
				response.set_server_ip(entity->ServerIp());
				response.set_server_port(entity->ServerPort());
			}
			else
			{
				entity->SetConnNum(-1);
			}

			

			dnServer->GetLogger()->Record(ELogLevel_Debug, response.DebugString());
		}

		co_return;
	}
}