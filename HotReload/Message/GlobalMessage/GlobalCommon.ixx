module;
#include "S_Common.pb.h"
#include "hv/Channel.h"

export module GlobalMessage:GlobalCommon;

import DNTask;
import MessagePack;
import GlobalServerHelper;
import ServerEntityHelper;

using namespace google::protobuf;
using namespace GMsg::S_Common;
using namespace hv;
using namespace std;

// client request
export void Msg_ReqRegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_ReqRegistSrv* requset = reinterpret_cast<COM_ReqRegistSrv*>(msg);
	COM_ResRegistSrv response;

	GlobalServerHelper* dnServer = GetGlobalServer();
	ServerEntityManagerHelper<ServerEntity>*  entityMan = dnServer->GetEntityManager();

	ServerType regType = (ServerType)requset->server_type();
	
	if(regType < ServerType::AuthServer || regType > ServerType::DedicatedServer)
	{
		response.set_success(false);
	}

	//exist?
	else if (ServerEntityHelper* entity = channel->getContext<ServerEntityHelper>())
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
			if (uint64_t timerId = child->TimerId())
			{
				child->TimerId() = 0;
				dnServer->GetSSock()->Timer()->killTimer(timerId);
			}

			// already connect
			if(auto sock = child->GetSock())
			{
				response.set_success(false);
			}
			else
			{
				entity->LinkNode() = nullptr;
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

			entity->ServerIp() = requset->ip();
			entity->ServerPort() = requset->port();
			
			for (int i = 0; i < requset->childs_size(); i++)
			{
				const COM_ReqRegistSrv& child = requset->childs(i);
				ServerType childType = (ServerType)child.server_type();
				ServerEntityHelper* servChild = entityMan->AddEntity(child.server_index(), childType);
				entity->SetMapLinkNode(childType, servChild);
			}
			

		}
		
	}

	else if (ServerEntityHelper* entity = entityMan->AddEntity(entityMan->ServerIndex(), regType))
	{
		entity->ServerIp() = requset->ip();
		entity->ServerPort() = requset->port();
		entity->GetChild()->SetSock(channel);

		channel->setContext(entity);

		response.set_success(true);
		response.set_server_index(entity->GetChild()->ID());
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

export void Exe_RetHeartbeat(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_RetHeartbeat* requset = reinterpret_cast<COM_RetHeartbeat*>(msg);
}
