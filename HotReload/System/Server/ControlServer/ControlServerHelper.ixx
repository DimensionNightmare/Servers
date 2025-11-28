export module ControlServerHelper;

import Server;
import ServerProxyHelper;
import ServerEntityManagerHelper;
import MessagePack;
import FuncUtils;
import Logger;
import ControlServerMessage;
import StrUtils;

export class ControlServerHelper : public Helper<ControlServerHelper, Server>
{
private:

	ControlServerHelper() = delete;
	~ControlServerHelper() = default;

public:

	ServerProxyHelper::Ptr GetServerProxy() 
	{
		return GetComponent<ServerProxyHelper>(EMComponentType::ServerProxy);
	}

	ServerEntityManagerHelper::Ptr GetServerEntityManager() 
	{
		return GetComponent<ServerEntityManagerHelper>(EMComponentType::ServerEntityManager);
	}

	
	void HandleServerInit()
	{

		if (ServerProxyHelper::CVPtr proxy = GetServerProxy())
		{
			proxy->onConnection = [this](SocketChannel::CVPtr channel)
				{
					World::CVPtr world = GetWorld();
					
					ServerProxyHelper::CVPtr proxyHelper = GetServerProxy();

					const std::string& peeraddr = channel->peeraddr();
					
					if (channel->isConnected())
					{
						LoggerPrint::Log(world, EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());
						proxyHelper->InitConnectedChannel(channel);

					}
					else
					{
						LoggerPrint::Log(world, EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());

						// not used
						if (ServerEntity::CVPtr entity = channel->GetEntity<ServerEntity>())
						{
							ServerEntityManagerHelper::CVPtr entityMan = GetServerEntityManager();
							entityMan->DisposeEntity(entity);
						}

					}
				};

			proxy->onMessage = [this](SocketChannel::CVPtr channel, hv::Buffer* buf)
				{
					MessagePacket* packet = MessagePacket::From(buf->data());

					World::CVPtr world = GetWorld();

					if(packet->pkgLenth > 2 * 1024)
					{
						LoggerPrint::Log(world, ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
						return;
					}
					
					std::string msgData(packet->MsgBegin(), packet->pkgLenth);
					
					if (packet->dealType == EMMsgDeal::Req)
					{
						ServerMessage::GetMessageHandle()->MsgHandle(world, channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Ret)
					{
						ServerMessage::GetMessageHandle()->MsgRetHandle(world, channel, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Redir)
					{
						ServerMessage::GetMessageHandle()->MsgRedirectHandle(world, channel, packet->msgId, packet->msgHashId, msgData);
					}
					else if (packet->dealType == EMMsgDeal::Res)
					{
						ServerProxyHelper::CVPtr proxyHelper = GetServerProxy();

						if (MsgTask* task = proxyHelper->GetMsg(packet->msgId)) //client sock request
						{
							proxyHelper->DelMsg(packet->msgId);

							if (Message* message = task->GetMessage())
							{
								if (!message->ParseFromString(msgData))
								{
									task->SetFlag(EMTaskFlag::PaserError);
								}

							}

							task->Resume();
						}
						else
						{
							LoggerPrint::Log(world, EL10nCode_MsgFind);
						}
					}
					else
					{
						LoggerPrint::Log(world, EL10nCode_MsgDealType);
					}
				};

		}

		return;
	}

	void HandleServerShutdown()
	{
		if (ServerProxyHelper::CVPtr proxy = GetServerProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;

			proxy->ClearMsgMap();
		}

		return;
	}

};
