module;
#include "google/protobuf/dynamic_message.h"

#include "hv/Channel.h"
#include "hv/hloop.h"
export module GlobalServerInit;

export import GlobalMessage;
import MessagePack;
import GlobalServer;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export void HandleGlobalServerInit(GlobalServer *server);
export void HandleGlobalServerShutdown(GlobalServer *server);

module:private;

void HandleGlobalServerInit(GlobalServer *server)
{
	SetGlobalServer(server);

	if (auto sSock = server->GetSSock())
	{
		auto onConnection = [&](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			if (channel->isConnected())
			{
				printf("%s connected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}
			else
			{
				printf("%s disconnected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
			}
		};

		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) {
			
		};

		sSock->onConnection = onConnection;
		sSock->onMessage = onMessage;
	}

	if (auto cSock = server->GetCSock())
	{
		auto onConnection = [](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			auto globalSrv = GetGlobalServer();

			if (channel->isConnected())
			{
				printf("%s connected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());

				// send RegistInfo
				// globalSrv->GetCSock()->ExecTaskByDll<int, DNClientProxy*, DNServerProxy*>(&Msg_RegistSrv, (int)ServerType::GlobalServer, globalSrv->GetCSock(), globalSrv->GetSSock());
				globalSrv->GetCSock()->RegistSelf<int, DNClientProxy*, DNServerProxy*>(&Msg_RegistSrv, (int)ServerType::GlobalServer, globalSrv->GetCSock(), globalSrv->GetSSock());
			}
			else
			{
				printf("%s disconnected! connfd=%d id=%d \n", peeraddr.c_str(), channel->fd(), channel->id());
				globalSrv->GetCSock()->SetRegisted(false);
			}

			if(globalSrv->GetCSock()->isReconnect())
			{
				
			}
		};
		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) {
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Res)
			{
				auto reqMap = GetGlobalServer()->GetCSock()->GetMsgMap();
				if(reqMap->contains(packet.msgId)) //client sock request
				{
					auto task = reqMap->at(packet.msgId);
					reqMap->erase(packet.msgId);
					task->Resume();
					Message* message = task->GetResult();
					message->ParseFromArray((const char*)buf->data() + MessagePacket::PackLenth + packet.msgLenth, packet.pkgLenth - packet.msgLenth);
					task->CallResume();
				}
				else
				{
					cout << "cant find msgid" << endl;
				}
			}
			else if(packet.dealType == MsgDeal::Req)
			{
				string msgName;
				msgName.resize(packet.msgLenth);
				memcpy(msgName.data(), (char*)buf->data() + MessagePacket::PackLenth, msgName.size());

				const Descriptor* descriptor = DescriptorPool::generated_pool()->FindMessageTypeByName(msgName); 
				if(descriptor)
				{
					static DynamicMessageFactory factory;
					const Message* prototype = factory.GetPrototype(descriptor);
					auto message = prototype->New();
					if(message->ParseFromArray((const char*)buf->data() + MessagePacket::PackLenth + msgName.size(), packet.pkgLenth - msgName.size()))
					{
						GlobalMessageHandle::MsgHandle(channel, packet.msgId, msgName, message);
					}
					else
					{
						cout << "cant deal msg parse" << endl;
					}
					delete message;
					message = nullptr;
				}
				else
				{
					cout << "cant find msg deal" << endl;
				}
			}
		};

		cSock->onConnection = onConnection;
		cSock->onMessage = onMessage;
		
	}

	// regist self if need
}

void HandleGlobalServerShutdown(GlobalServer *server)
{
	if (auto sSock = server->GetSSock())
	{
		sSock->onConnection = nullptr;
		sSock->onMessage = nullptr;
	}

	if (auto cSock = server->GetCSock())
	{
		for(auto& kv : *cSock->GetMsgMap())
		{
			kv.second->Destroy();
		}
		cSock->GetMsgMap()->clear();
		cSock->onConnection = nullptr;
		cSock->onMessage = nullptr;
	}
}