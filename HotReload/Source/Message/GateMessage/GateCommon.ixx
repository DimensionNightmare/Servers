module;
#include "Common.pb.h"
#include "hv/Channel.h"

#include <coroutine>
export module GateMessage:GateCommon;

import DNTask;
import MessagePack;
import GateServerHelper;
import AfxCommon;
import ServerEntityHelper;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::Common;

// client request
export DNTaskVoid Msg_RegistSrv()
{
	auto dnServer = GetGateServer();
	auto client = dnServer->GetCSock();
	auto server = dnServer->GetSSock();
	auto msgId = client->GetMsgId();
	
	// first Can send Msg?
	if(client->GetMsg(msgId))
	{
		DNPrintErr("+++++ %lu, \n", msgId);
		co_return;
	}
	else
	{
		DNPrint("Msg_RegistSrv ----- %lu, \n", msgId);
	}

	COM_ReqRegistSrv requset;
	requset.set_server_type((int)dnServer->GetServerType());
	if(server->host == "0.0.0.0")
	{
		requset.set_ip("127.0.0.1");
	}
	else
	{
		requset.set_ip(server->host);
	}
	requset.set_port(server->port);
	requset.set_server_index(dnServer->GetServerIndex());
	
	// pack data
	string binData;
	binData.resize(requset.ByteSizeLong());
	requset.SerializeToArray(binData.data(), (int)binData.size());
	MessagePack(msgId, MsgDeal::Req, requset.GetDescriptor()->full_name(), binData);
	
	// data alloc
	COM_ResRegistSrv response;
	auto dataChannel = [&response]()->DNTask<Message*>
	{
		co_return &response;
	}();


	client->AddMsg(msgId, (DNTask<void*>*)&dataChannel);
	
	// wait data parse
	client->send(binData);
	co_await dataChannel;
	
	if(!response.success())
	{
		DNPrint("regist Server error! msg:%lu \n", msgId);
		// dnServer->SetRun(false); //exit application
	}
	else
	{
		DNPrint("regist Server success! \n");
		if(dnServer->GetServerIndex() == 0 && response.server_index())
		{

		}
		client->SetRegisted(true);
	}

	dataChannel.Destroy();

	co_return;
}

// client request
export void Exe_RegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_ReqRegistSrv* requset = (COM_ReqRegistSrv*)msg;
	COM_ResRegistSrv response;

	auto entityMan = GetGateServer()->GetEntityManager();

	ServerType regType = (ServerType)requset->server_type();
	
	if(regType < ServerType::GateServer || regType >= ServerType::Max)
	{
		response.set_success(false);
	}

	//exist?
	if (auto entity = channel->getContext<ServerEntityHelper>())
	{
		response.set_success(false);
	}

	// take task to regist !
	else if (requset->server_index())
	{
		auto entity = entityMan->GetEntity<ServerEntityHelper>(requset->server_index());
		if (entity)
		{
			auto child = entity->GetChild();
			// wait destroy`s destroy
			if (auto timerId = child->GetTimerId())
			{
				child->SetTimerId(0);
				GetGateServer()->GetSSock()->loop()->killTimer(timerId);
			}

			// already connect
			if(auto sock = child->GetSock())
			{
				response.set_success(false);
			}
			else
			{
				entity->SetLinkNode(nullptr);
				child->SetSock(channel);
				response.set_success(true);
			}
		}
		else
		{
			response.set_success(false);
		}
		
	}

	else if (auto entity = entityMan->AddEntity<ServerEntityHelper>(channel, entityMan->GetServerIndex(), regType))
	{
		response.set_success(true);
		response.set_server_index(entity->GetChild()->GetID());
		entity->SetServerIp(requset->ip());
		entity->SetServerPort(requset->port());
	}
	
	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, "", binData);
	channel->write(binData);
}