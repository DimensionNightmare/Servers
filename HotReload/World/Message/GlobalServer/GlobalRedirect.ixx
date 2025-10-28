export module GlobalServerMessage:GlobalRedirect;

import FuncHelper;
import GlobalServerHelper;
import ThirdParty.Libhv;
import Task;
import ServerEntity;
import ThirdParty.PbGen;
import Logger;
import GlobalServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::A2g_ReqAuthAccount, GMsg::g2A_ResAuthAccount, EMMsgDeal::Redir> Msg_ReqAuthAccount(
				[](auto request, auto response, SocketChannel::Ptr channel) -> TaskVoid
	{
		// if has db not need origin
		GlobalServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GlobalServerHelper>(EMSystemType::Server);
		std::list<ServerEntity::Ptr> serverList = dnServer->GetServerEntityManager()->GetEntitysByType(EMServerType::GateServer);

		std::list<ServerEntityHelper::Ptr> tempList;
		for (ServerEntity::Ptr& server : serverList)
		{
			if (server->HasFlag(EMServerEntityFlag::Locked))
			{
				tempList.emplace_back(server->GetSelf<ServerEntityHelper>());
			}
		}

		tempList.sort([](ServerEntityHelper::Ptr lhs, ServerEntityHelper::Ptr rhs) { return lhs->ConnNum() < rhs->ConnNum(); });

		if (tempList.empty())
		{
			response->set_errorcode(EL10nCode_NotExistGateServer);
		}
		else
		{
			ServerEntityHelper::Ptr entity = tempList.front();
			LoggerPrint::Log(channel, ELogLevel_Debug, "send to GateServer : {}", entity->ID());

			entity->SetConnNum(1);

			// data alloc
			auto taskGen = [](Message* msg) -> Task<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(response);

			ServerProxyHelper::Ptr serverProxy = dnServer->GetServerProxy();
			uint32_t msgId = serverProxy->GetMsgId();

			serverProxy->AddMsg(msgId, &dataChannel, 8000);
			
			std::string binMsg;
			request->SerializeToString(&binMsg);
			MessagePackAndSend(msgId, EMMsgDeal::Req, request->GetDescriptor()->full_name(), binMsg, entity->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMTaskFlag::Timeout))
			{
				response->set_errorcode(EL10nCode_SGlobalReqTimeout);

			}

			if(response->errorcode() == EL10nCode_None)
			{
				response->set_serverip(entity->ServerIp());
				response->set_serverport(entity->ServerPort());
			}
			else
			{
				entity->SetConnNum(-1);
			}

			

			LoggerPrint::Log(channel, ELogLevel_Debug, "Msg_ReqAuthAccount:{}", response->DebugString());
		}

		co_return;
	});
}