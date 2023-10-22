module;

#include "hv/Channel.h"
#include "GlobalControl.pb.h"

#include <coroutine>
export module GlobalControl;

import DNTask;
import DNServer;
import MessagePack;

using namespace GMsg::GlobalControl;
using namespace std;
using namespace google::protobuf;

// client request
export void Msg_RegistSrv(int serverType, DNClientProxy* client, DNServerProxy* server)
{
	auto taskChannel = [&]()-> DNTaskVoid
	{
		// coroutine need this after await
		auto type = serverType;
		auto tempCli = client;
		auto tempSrv = server;
		auto msgId = tempCli->GetMsgId();
		auto reqMap = tempCli->GetMsgMap();

		G2C_RegistSrv requset;
		requset.set_server_type(type);
		requset.set_ip(tempSrv->host);
		requset.set_port(tempSrv->port);
		
		// pack data
		string binData;
		binData.resize(requset.ByteSize());
		requset.SerializeToArray(binData.data(), binData.size());
		MessagePack(msgId, MsgDir::Inner, G2C_RegistSrv::GetDescriptor()->full_name(), binData);
		
		// data alloc
		C2G_RegistSrv response;
		auto dataChannel = [&]()->DNTask<Message*>
			{
				co_return &response;
			}();
		if(reqMap->contains(msgId))
		{
			cout << "+++++++++++++++++++++++++++++++++++++++" <<endl;
		}
		reqMap->emplace(msgId, &dataChannel);
		
		// wait data parse
		tempCli->send(binData);
		co_await dataChannel;
		reqMap->erase(msgId);
		
		if(!response.success())
		{
			cout << "regist false" << (int)msgId << endl;
			Msg_RegistSrv(type, tempCli, tempSrv);
		}
		else{
			cout << "regist true" << (int)msgId << endl;
		}
		dataChannel.Destroy();

		co_return;
	}();

	
}