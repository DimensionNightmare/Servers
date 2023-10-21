module;
#include "hv/Channel.h"
#include "hv/hloop.h"
#include "google/protobuf/dynamic_message.h"
export module ControlServerInit;

export import ControlMessage;
import MessagePack;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export void HandleControlServerInit(ControlServer *server);
export void HandleControlServerShutdown(ControlServer *server);

module:private;

void HandleControlServerInit(ControlServer *server)
{
	GetControlServer(server);
	
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
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.opType == MsgDir::Inner)
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
						ControlMessageHandle::MsgHandle(channel, packet.msgId, msgName, message);
					}
				}
			}
		};

		sSock->onConnection = onConnection;
		sSock->onMessage = onMessage;

		ControlMessageHandle::RegMsgHandle();
	}
}

void HandleControlServerShutdown(ControlServer *server)
{
	if (auto sSock = server->GetSSock())
	{
		sSock->onConnection = nullptr;
		sSock->onMessage = nullptr;
	}
}