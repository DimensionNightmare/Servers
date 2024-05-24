module;
#include <cstdint>
#include "hv/Channel.h"

#include "Server/S_Common.pb.h"
export module ControlMessage:ControlCommon;

import DNTask;
import MessagePack;
import ControlServerHelper;

using namespace google::protobuf;
using namespace GMsg;
using namespace hv;
using namespace std;

namespace ControlMessage
{

	// client request
	export void Msg_ReqRegistSrv(SocketChannelPtr channel, uint32_t msgId, Message *msg)
	{
		COM_ReqRegistSrv* requset = reinterpret_cast<COM_ReqRegistSrv*>(msg);
		COM_ResRegistSrv response;

		ServerEntityManagerHelper*  entityMan = GetControlServer()->GetServerEntityManager();

		ServerType regType = (ServerType)requset->server_type();

		const string& ipPort = channel->localaddr();
		
		if(regType < ServerType::GlobalServer || regType > ServerType::AuthServer || ipPort.empty())
		{
			response.set_success(false);
		}

		//exist?
		else if (ServerEntityHelper* entity = channel->getContext<ServerEntityHelper>())
		{
			response.set_success(false);
		}

		else if (ServerEntityHelper* entity = entityMan->AddEntity(entityMan->ServerIndex(), regType))
		{
			size_t pos = ipPort.find(":");
			entity->ServerIp() = ipPort.substr(0, pos);
			entity->ServerPort() = requset->port();
			entity->SetSock(channel);

			channel->setContext(entity);

			response.set_success(true);
			response.set_server_index(entity->ID());
		}

		string binData;
		binData.resize(response.ByteSizeLong());
		response.SerializeToArray(binData.data(), binData.size());

		MessagePack(msgId, MsgDeal::Res, nullptr, binData);
		channel->write(binData);
	}

	export void Exe_RetHeartbeat(SocketChannelPtr channel, uint32_t msgId, Message *msg)
	{
		COM_RetHeartbeat* requset = reinterpret_cast<COM_RetHeartbeat*>(msg);
	}
}
