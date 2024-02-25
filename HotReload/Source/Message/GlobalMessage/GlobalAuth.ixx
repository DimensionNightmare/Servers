module;
#include "AuthGlobal.pb.h"
#include "GlobalControl.pb.h"
#include "hv/Channel.h"

#include <coroutine>
#include <random>
export module GlobalMessage:GlobalAuth;

import DNTask;
import MessagePack;
import GlobalServerHelper;
import ServerEntityHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::AuthGlobal;

#define CastObj(entity) static_cast<ServerEntityHelper*>(entity)

export DNTaskVoid Exe_AuthAccount(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	A2G_AuthAccount* requset = (A2G_AuthAccount*)msg;
	G2A_AuthAccount response;

	ServerEntity* servEntity = nullptr;

	// if has db not need origin
	list<ServerEntity*> servList = GetGlobalServer()->GetEntityManager()->GetEntityByList(ServerType::GateServer);
	if(servList.size())
	{
		//random
		random_device rd;
		mt19937 gen(rd());
		uniform_int_distribution<> dis(0, servList.size() - 1);
		int randomIndex = dis(gen);

		auto it = servList.begin();
    	advance(it, randomIndex);
		servEntity = *it;
	}

	if(servEntity)
	{

		co_return;
	}

	servList = GetGlobalServer()->GetEntityManager()->GetEntityByList(ServerType::GateServer);
	if(servList.size())
	{
		for(ServerEntity* it : servList)
		{
			ServerEntityHelper* gate = CastObj(it);
			if(!gate->GetMapLinkNode(ServerType::DatabaseServer).size())
			{
				servList.remove(it);
			}

		}
		
	}



	co_return;
}