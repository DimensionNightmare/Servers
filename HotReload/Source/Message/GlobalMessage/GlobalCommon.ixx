module;
#include "CommonMsg.pb.h"
#include "hv/Channel.h"

export module GlobalMessage:GlobalCommon;

import DNTask;
import MessagePack;
import GlobalServerHelper;
import ServerEntityHelper;

using namespace google::protobuf;
using namespace GMsg::CommonMsg;
using namespace hv;
using namespace std;

// client request
export void Exe_RegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_ReqRegistSrv* requset = (COM_ReqRegistSrv*)msg;
	COM_ResRegistSrv response;

	GlobalServerHelper* dnServer = GetGlobalServer();
	auto entityMan = dnServer->GetEntityManager();

	ServerType regType = (ServerType)requset->server_type();
	
	if(regType < ServerType::AuthServer || regType > ServerType::LogicServer)
	{
		response.set_success(false);
	}

	//exist?
	if (ServerEntityHelper* entity = channel->getContext<ServerEntityHelper>())
	{
		response.set_success(false);
	}

	// take task to regist !
	else if (requset->server_index())
	{
		ServerEntityHelper* entity = entityMan->GetEntity(requset->server_index());
		if (entity)
		{
			auto child = entity->GetChild();
			// wait destroy`s destroy
			if (uint64_t timerId = child->GetTimerId())
			{
				child->SetTimerId(0);
				dnServer->GetSSock()->loop(0)->killTimer(timerId);
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

				// Re-enroll
				entityMan->MountEntity(regType, entity);
			}
		}
		else
		{
			response.set_success(true);
			response.set_server_index(requset->server_index());

			entity = entityMan->AddEntity(requset->server_index(), regType);
			entity->GetChild()->SetSock(channel);

			channel->setContext(entity);

			entity->SetServerIp(requset->ip());
			entity->SetServerPort(requset->port());
			
			for (int i = 0; i < requset->childs_size(); i++)
			{
				const COM_ReqRegistSrv& child = requset->childs(i);
				ServerType childType = (ServerType)child.server_type();
				ServerEntityHelper* servChild = entityMan->AddEntity(child.server_index(), childType);
				entity->SetMapLinkNode(childType, servChild);
			}
			

		}
		
	}

	else if (ServerEntityHelper* entity = entityMan->AddEntity(entityMan->GetServerIndex(), regType))
	{
		entity->SetServerIp(requset->ip());
		entity->SetServerPort(requset->port());
		entity->GetChild()->SetSock(channel);

		channel->setContext(entity);

		response.set_success(true);
		response.set_server_index(entity->GetChild()->GetID());
	}
	
	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	channel->write(binData);

	if(!response.success())
	{
		return;
	}

	dnServer->UpdateServerGroup();
}