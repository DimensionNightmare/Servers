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
	export void Msg_ReqRegistSrv(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		COM_ReqRegistSrv* request = reinterpret_cast<COM_ReqRegistSrv*>(msg);
		COM_ResRegistSrv response;

		ControlServerHelper* dnServer = GetControlServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();

		ServerType regType = (ServerType)request->server_type();

		const string& ipPort = channel->localaddr();

		if (regType < ServerType::GlobalServer || regType > ServerType::AuthServer || ipPort.empty())
		{
			response.set_success(false);
		}

		//exist?
		else if (ServerEntity* entity = channel->getContext<ServerEntity>())
		{
			response.set_success(false);
		}

		else if (ServerEntity* entity = entityMan->AddEntity(entityMan->ServerIndex(), regType))
		{
			size_t pos = ipPort.find(":");
			entity->ServerIp() = ipPort.substr(0, pos);
			entity->ServerPort() = request->port();
			entity->SetSock(channel);

			channel->setContext(entity);

			response.set_success(true);
			response.set_server_index(entity->ID());
			response.set_server_type((uint8_t(dnServer->GetServerType())));
		}

		string binData;
		response.SerializeToString(&binData);

		MessagePack(msgId, MsgDeal::Res, nullptr, binData);
		channel->write(binData);
	}

	export void Exe_RetHeartbeat(SocketChannelPtr channel, Message* msg)
	{
		COM_RetHeartbeat* request = reinterpret_cast<COM_RetHeartbeat*>(msg);
	}
}
