module;
export module DatabaseMessage:DatabaseCommon;

import DNTask;
import FuncHelper;
import Logger;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNClientProxyHelper;
import DNServer;
import DatabaseServerHelper;

#define FUNCPLACE(func) #func, func

namespace DatabaseMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv(const DNServer::Ptr& server)
	{
		DatabaseServerHelper::Ptr dnServer = server->GetSelf<DatabaseServerHelper>();

		DNClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();
		
		dnServer->GetLogger()->Record(ELogLevel_Debug, "database req regist Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);
		
		clientProxy->SetRegistState(EMRegistState::Registing);

		GMsg::COM_ReqRegistSrv request;

		request.set_server_type((int)dnServer->GetServerType());

		if (uint32_t serverIndex = dnServer->ServerId())
		{
			request.set_server_id(serverIndex);
		}

		// data alloc
		GMsg::COM_ResRegistSrv response;

		{
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			uint32_t msgId = clientProxy->GetMsgId();
			clientProxy->AddMsg(msgId, &dataChannel);
			// pack data
			std::string binData;
			request.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binData, clientProxy->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				dnServer->GetLogger()->Record(ELogLevel_Debug, "requst timeout! ");
			}

		}

		if (response.success())
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "regist Server success! Rec index:{}", response.server_id());
			clientProxy->SetRegistState(EMRegistState::Registed);
			clientProxy->SetRegistType(response.server_type());
			dnServer->SetServerId(response.server_id());
		}
		else
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "regist Server error!  ");
			// dnServer->IsRun() = false; //exit application
			clientProxy->SetRegistState(EMRegistState::None);
		}

		co_return;
	}

	export void Exe_RetChangeCtlSrv(const World::Ptr& world, const DNSocketProxy::Ptr& channel, std::string binMsg)
	{
		GMsg::COM_RetChangeCtlSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		DNServer::Ptr dnServer = world->GetSystem<DNServer>(EMSystemType::DNServer);

		DNClientProxy::Ptr clientProxy = dnServer->GetComponent<DNClientProxy>(EMComponentType::DNClientProxy);

		TickMainSpaceDll(clientProxy.get(), FUNCPLACE(&DNClientProxy::RedirectClient), request.server_port(), request.server_ip());
	}
}
