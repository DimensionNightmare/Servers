module;

#include "GlobalControl.pb.h"
#include "hv/Channel.h"
export module ControlGlobal;

import DNTask;
import MessagePack;
import ControlServerHelper;
import ServerEntityHelper;

using namespace GMsg::GlobalControl;
using namespace google::protobuf;
using namespace hv;
using namespace std;

// client request
export void Msg_RegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	C2G_RegistSrv response;
	response.set_success(true);
	
	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, "", binData);
	channel->write(binData);
	
	auto glbS = GetControlServer();
	auto entityOrgin = glbS->GetEntityManager()->AddEntity(channel, channel->id());
	// entityOrgin->SetServerType(ServerType::ControlServer);
	ServerEntityHelper* entity = static_cast<ServerEntityHelper*>(entityOrgin);
	// if(entity)
	// {
		entity->SetServerType(ServerType::ControlServer);
		// entity->GetSelf()->SetID(channel->id());
		// entity->GetSelf()->SetSock(channel);
	// }
	// auto a1 = static_cast<ServerEntity*>(entity);
	// auto a2 = static_cast<Entity*>(a1);
	// auto a3 = static_cast<EntityHelper*>(a2);
	// if(a3)
	// {
	// 	a3->SetID(channel->id());
	// 	a3->SetSock(channel);
	// }
}