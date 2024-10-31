module;
#include "StdMacro.h"
export module GateMessage:GateCommon;

import DNTask;
import FuncHelper;
import GateServerHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ServerEntityManagerHelper;

namespace GateMessage
{

	void Evt_RetRegistChild()
	{
		GateServerHelper* dnServer = GetGateServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();
		DNClientProxyHelper* client = dnServer->GetCSock();

		g2G_RetRegistChild request;

		request.set_server_id(dnServer->ServerId());

		auto AddChild = [&request](ServerEntity* serv)
			{
				COM_ReqRegistSrv* child = request.add_childs();
				child->set_server_id(serv->ID());
				child->set_server_type((uint32_t)serv->GetServerType());
			};

		const list<ServerEntity*>& dbs = entityMan->GetEntitysByType(EMServerType::DatabaseServer);
		for (ServerEntity* serv : dbs)
		{
			AddChild(serv);
		}

		const list<ServerEntity*>& logics = entityMan->GetEntitysByType(EMServerType::LogicServer);
		for (ServerEntity* serv : logics)
		{
			AddChild(serv);
		}

		if( !request.childs_size())
		{
			return;
		}

		// pack data
		string binData;
		request.SerializeToString(&binData);
		MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name().c_str(), binData, client->GetChannel());
	}

	// self request
	export DNTaskVoid Evt_ReqRegistSrv()
	{
		GateServerHelper* dnServer = GetGateServer();
		DNClientProxyHelper* client = dnServer->GetCSock();
		DNServerProxy* server = dnServer->GetSSock();
		
		DNPrint(0, EMLoggerLevel::Debug, "Client:%s, port:%hu", client->remote_host.c_str(), client->remote_port);
		
		client->EMRegistState() = EMRegistState::Registing;

		COM_ReqRegistSrv request;

		request.set_server_type((int)dnServer->GetServerType());

		if (uint32_t serverId = dnServer->ServerId())
		{
			request.set_server_id(serverId);
		}

		request.set_server_port(server->port);

		// pack data
		string binData;
		request.SerializeToString(&binData);
		

		// data alloc
		COM_ResRegistSrv response;

		{
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);
			
			uint32_t msgId = client->GetMsgId();
			client->AddMsg(msgId, &dataChannel);
			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name().c_str(), binData, client->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				DNPrint(0, EMLoggerLevel::Debug, "requst timeout! ");
			}

		}

		if (response.success())
		{
			DNPrint(0, EMLoggerLevel::Debug, "regist Server success! Rec index:%d", response.server_id());
			client->EMRegistState() = EMRegistState::Registed;
			client->RegistType() = response.server_type();
			dnServer->ServerId() = response.server_id();

			Evt_RetRegistChild();
		}
		else
		{
			DNPrint(0, EMLoggerLevel::Debug, "regist Server error!");
			// dnServer->IsRun() = false; //exit application
			client->EMRegistState() = EMRegistState::None;
		}

		co_return;
	}

	// client request
	export void Msg_ReqRegistSrv(SocketChannelPtr channel, uint32_t msgId,  string binMsg)
	{
		COM_ReqRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		DNPrint(0, EMLoggerLevel::Debug, "ip Reqregist: %s, %d", channel->peeraddr().c_str(), request.server_type());

		COM_ResRegistSrv response;

		GateServerHelper* dnServer = GetGateServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();

		EMServerType regType = (EMServerType)request.server_type();
		uint32_t serverId = request.server_id();

		const string& ipPort = channel->localaddr();

		if (regType < EMServerType::DatabaseServer || regType > EMServerType::LogicServer || ipPort.empty())
		{
			response.set_success(false);
		}

		//exist?
		if (ServerEntity* entity = channel->getContext<ServerEntity>())
		{
			response.set_success(false);
		}

		else if (ServerEntity* entity = entityMan->AddEntity(serverId, regType))
		{
			size_t pos = ipPort.find(":");
			entity->ServerIp() = ipPort.substr(0, pos);
			entity->ServerPort() = request.server_port();
			entity->SetSock(channel);

			channel->setContext(entity);

			response.set_success(true);
			response.set_server_id(entity->ID());
			response.set_server_type((uint8_t(dnServer->GetServerType())));
		}
		else
		{
			abort();
		}

		string binData;
		response.SerializeToString(&binData);

		MessagePackAndSend(msgId, EMMsgDeal::Res, nullptr, binData, channel);

		if (response.success())
		{
			// up to Global
			g2G_RetRegistSrv request;
			request.set_is_regist(true);
			request.set_server_id(serverId);

			request.SerializeToString(&binData);
			

			GateServerHelper* dnServer = GetGateServer();
			DNClientProxyHelper* client = dnServer->GetCSock();

			MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name().c_str(), binData, client->GetChannel());
		}
	}

	export void Exe_RetHeartbeat(SocketChannelPtr channel, string binMsg)
	{
		COM_RetHeartbeat request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
	}
}
