module;
export module GateMessage:GateClient;

import DNTask;
import StrUtils;
import FuncHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ProxyEntityManagerHelper;
import std.compat;
import ServerEntity;
import GateServerHelper;
import ECSW;

namespace GateMessage
{

	// client request
	export DNTaskVoid Msg_ReqAuthToken(const DNSocketChannel::Ptr& channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::C2S_ReqAuthToken request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}

		GateServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::DNServer);
		ProxyEntityManagerHelper::Ptr entityMan = dnServer->GetProxyEntityManager();

		GMsg::S2C_ResAuthToken response;

		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
		});
		

		ProxyEntity::Ptr entity = entityMan->GetEntity(request.account_id());
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

			channel->setContextPtr(entity);
			entity->SetChannel(channel);

			if (uint64_t timerId = entity->TimerId())
			{
				entity->SetTimerId(0);
				entityMan->Timer()->killTimer(timerId);
			}

			//DS Server
			ServerEntityManagerHelper::Ptr serverEntityMan = dnServer->GetServerEntityManager();
			ServerEntity::Ptr serverEntity = nullptr;

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
					serverEntity = serverEntityList.front();
				}
			}

			//req dedicatedServer Info to Login ds.
			if (serverEntity)
			{
				entity->SetRecordServerId(serverEntity->ID());

				auto taskGen = [](Message* msg) -> DNTask<Message*>
					{
						co_return msg;
					};
				auto dataChannel = taskGen(&response);

				DNServerProxyHelper::Ptr server = dnServer->GetServerProxy();
				uint32_t msgId = server->GetMsgId();
				server->AddMsg(msgId, &dataChannel, 9000);

				MessagePackAndSend(msgId, EMMsgDeal::Redir, request.GetDescriptor()->full_name(), binMsg, serverEntity->GetChannel());
				
				co_await dataChannel;
				if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
				{
					response.set_error_code(EL10nCode_SGateReqTimeout);
				}

			}

		}

		co_return;
	}
}
