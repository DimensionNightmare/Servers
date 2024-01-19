module;
#include "Common.pb.h"
#include "hv/Channel.h"

export module GlobalMessage:GlobalCommon;

import DNTask;
import MessagePack;
import GlobalServerHelper;
import ServerEntityHelper;

using namespace google::protobuf;
using namespace GMsg::Common;
using namespace hv;
using namespace std;

// client request
export void EXE_Msg_RegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_ReqRegistSrv* requset = (COM_ReqRegistSrv*)msg;
	COM_ResRegistSrv response;

	auto entityMan = GetGlobalServer()->GetEntityManager();

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
				GetGlobalServer()->GetSSock()->loop()->killTimer(timerId);
			}

			// already connect
			if(auto sock = child->GetSock())
			{
				response.set_success(false);
			}
			else
			{
				child->SetSock(channel);
				response.set_success(true);
			}
		}
		else
		{
			response.set_success(false);
		}
		
	}

	else if (auto entity = entityMan->AddEntity<ServerEntityHelper>(channel, entityMan->GetServerIndex(), (ServerType)requset->server_type()))
	{
		response.set_success(true);
	}
	
	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, "", binData);
	channel->write(binData);
}