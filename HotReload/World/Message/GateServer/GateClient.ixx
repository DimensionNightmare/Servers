export module GateServerMessage:GateClient;

import GateServerHelper;

import StrUtils;
import ThirdParty.Libhv;
import FuncHelper;
import Task;
import ProxyEntityHelper;
import std;

namespace GateServerMessage
{

	// client request
	export TaskVoid Msg_ReqAuthToken(SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::C2S_ReqAuthToken request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}

		GateServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::Server);
		ProxyEntityManagerHelper::CVPtr entityMan = dnServer->GetProxyEntityManager();

		GMsg::S2C_ResAuthToken response;

		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
		});
		

		ProxyEntityHelper::CVPtr entity = entityMan->GetEntity(request.account_id());
		if (!entity)
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "noaccount {}!!", request.account_id());
			response.set_error_code(EL10nCode_NoneProxyEntity);
		}
		// if not match, timer will destory entity
		else if (Md5Hash(entity->Token()) != request.token())
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "not match!!");
			response.set_error_code(EL10nCode_LoginTokenNotMatch);
		}
		else
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "match!!");

			if (uint64_t timerId = entity->TimerId())
			{
				entity->SetTimerId(0);
				entityMan->RemoveTimerRecord(timerId);
			}
			
			channel->setContextPtr(entity);
			entity->SetChannel(channel);


			//DS Server
			ServerEntityManagerHelper::Ptr serverEntityMan = dnServer->GetServerEntityManager();
			ServerEntityHelper::Ptr serverEntity = nullptr;

			// <cache> server to load login data
			if (uint64_t serverId = entity->RecordServerId())
			{
				serverEntity = serverEntityMan->GetEntity(serverId);
			}

			// pool
			if (!serverEntity)
			{
				std::list<ServerEntity::Ptr> serverEntityList = serverEntityMan->GetEntitysByType(EMServerType::LogicServer);
				if (serverEntityList.empty())
				{
					dnServer->GetLogger()->Record(ELogLevel_Debug, "Msg_ReqAuthToken not LogicServer !!");
					response.set_error_code(EL10nCode_NotExistLogicServer);
				}
				else
				{
					serverEntity = serverEntityList.front()->GetSelf<ServerEntityHelper>();
				}
			}

			//req dedicatedServer Info to Login ds.
			if (serverEntity)
			{
				entity->SetRecordServerId(serverEntity->ID());

				auto taskGen = [](Message* msg) -> Task<Message*>
					{
						co_return msg;
					};
				auto dataChannel = taskGen(&response);

				ServerProxyHelper::Ptr server = dnServer->GetServerProxy();
				uint32_t msgId = server->GetMsgId();
				server->AddMsg(msgId, &dataChannel, 9000);

				MessagePackAndSend(msgId, EMMsgDeal::Redir, request.GetDescriptor()->full_name(), binMsg, serverEntity->GetChannel());
				
				co_await dataChannel;
				if (dataChannel.HasFlag(EMTaskFlag::Timeout))
				{
					response.set_error_code(EL10nCode_SGateReqTimeout);
				}

			}

		}

		co_return;
	}
}
