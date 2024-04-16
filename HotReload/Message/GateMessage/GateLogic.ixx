module;
#include "S_Dedicated.pb.h"
#include "hv/Channel.h"

#include <coroutine>
export module GateMessage:GateLogic;

import GateServerHelper;
import DNTask;
import Utils.StrUtils;
import MessagePack;

using namespace std;
using namespace GMsg::S_Dedicated;
using namespace google::protobuf;
using namespace hv;

// client request
export void Msg_ReqAuthToken(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	D2G_ReqAuthToken* requset = reinterpret_cast<D2G_ReqAuthToken*>(msg);

	GateServerHelper* dnServer = GetGateServer();
	ProxyEntityManagerHelper<ProxyEntity>* entityMan = dnServer->GetProxyEntityManager();

	G2D_ResAuthToken response;

	if(ProxyEntityHelper* entity = entityMan->GetEntity(requset->account_id()))
	{
		response.set_success( Md5Hash(entity->Token()) == requset->token());
	}
	else
	{
		response.set_success(false);
	}

	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	
	channel->write(binData);
}