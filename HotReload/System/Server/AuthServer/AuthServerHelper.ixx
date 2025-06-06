module;
export module AuthServerHelper;

export import ThirdParty.PbGen;
import DNClientProxyHelper;
import DNWebProxyHelper;
import DbUtils;
import RdbProxyHelper;
import StrUtils;
import DNServer;
import DllUtils;
import MessagePack;
import ECSW;
import MessageRegister;

#define FUNCPLACE(class, func) &class::func, #class"_"#func


export class AuthServerHelper : public DNServer
{

private:

	AuthServerHelper() = delete;
	~AuthServerHelper() = default;

	AuthServerHelper(const AuthServerHelper&) = delete;
	// void operator=(const AuthServerHelper&) = delete;

	AuthServerHelper(AuthServerHelper&&) = delete;
	AuthServerHelper& operator=(AuthServerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<AuthServerHelper>;

	DNClientProxyHelper::Ptr GetClientProxy() { return GetComponent<DNClientProxyHelper>(EMComponentType::DNClientProxy); }

	DNWebProxyHelper::Ptr GetWebProxy() { return GetComponent<DNWebProxyHelper>(EMComponentType::DNWebProxy); }

	RdbProxyHelper::Ptr GetRdbProxy(){ return GetComponent<RdbProxyHelper>(EMComponentType::RdbProxy); }

	bool InitDatabase()
	{
		
		if(RdbProxyHelper::Ptr proxy = GetRdbProxy())
		{
			try
			{
				World::Ptr pWorld = GetWorld();

				//"postgresql://root@localhost"
				std::string* value = pWorld->LaunchParam("connection");

				std::string* dbName = pWorld->LaunchParam("dbname");

				auto connection = std::make_shared<pqxx::connection>(std::format("{} dbname = {}", *value, *dbName));

				proxy->AddConnection((uint16_t)EnumName<EMSqlDbNameEnum>(*dbName), std::move(connection));
			}
			catch (const std::exception& e)
			{
				GetLogger()->Record(ELogLevel_Debug, "{}", e.what());
				return false;
			}

			return true;
		}

		return false;
	}

	int HandleServerInit(MessageRegister* msgHandle)
	{
		msgHandle->RegMsgHandle();
		
		if (DNWebProxy::Ptr proxy = GetComponent<DNWebProxy>(EMComponentType::DNWebProxy))
		{
			// msgHandle->RegApiHandle(GetSelfW<DNServer>(), proxy->service);
		}

		if (DNClientProxy::Ptr proxy = GetComponent<DNClientProxy>(EMComponentType::DNClientProxy))
		{
			proxy->onConnection = [this,msgHandle](const DNSocketChannel::Ptr& channel)
				{
					DNClientProxyHelper::Ptr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}

					const std::string& peeraddr = channel->peeraddr();

					if (channel->isConnected())
					{
						GetLogger()->Record(EL10nCode_CliConnOn, peeraddr, channel->fd(), channel->id());

						channel->SetWorld(GetWorldW());

						proxyHelper->SetRegistEvent(msgHandle->GetClientRegistFunc());
						TickMainSpaceDll(proxyHelper.get(), FUNCPLACE(DNClientProxy,InitConnectedChannel),  channel);
					}
					else
					{
						GetLogger()->Record(EL10nCode_CliConnOff, peeraddr, channel->fd(), channel->id());

						if (proxyHelper->GetRegistState() == EMRegistState::Registed)
						{
							proxyHelper->SetRegistState(EMRegistState::None);
						}

						proxyHelper->SetRegistType(0);
					}

					if (proxyHelper->isReconnect())
					{

					}
				};

			proxy->onMessage = [this](const DNSocketChannel::Ptr& channel, hv::Buffer* buf)
				{
					DNClientProxyHelper::Ptr proxyHelper = GetClientProxy();

					if(!proxyHelper){ return ;}
					
					MessagePacket* packet = MessagePacket::From(buf->data());

					GetLogger()->Record(ELogLevel_Debug, "c {} Recv type={} With Mid:{}", channel->peeraddr(), static_cast<int>(packet->dealType), packet->msgId);

					if(packet->pkgLenth > 2 * 1024)
					{
						GetLogger()->Record(ELogLevel_Debug, "Recv byte len limit={}", packet->pkgLenth);
						return;
					}

					std::string msgData(packet->MsgBegin(), packet->pkgLenth);

					if (packet->dealType == EMMsgDeal::Res)
					{
						if (DNTask<Message*>* task = proxyHelper->GetMsg(packet->msgId)) //client sock request
						{
							proxyHelper->DelMsg(packet->msgId);
							task->Resume();

							if (Message* message = task->GetResult())
							{
								if (!message->ParseFromString(msgData))
								{
									task->SetFlag(EMDNTaskFlag::PaserError);
								}
							}

							task->CallResume();
						}
						else
						{
							GetLogger()->Record(EL10nCode_MsgFind);
						}
					}
					else
					{
						GetLogger()->Record(EL10nCode_MsgDealType);
					}
				};

		}
		

		return InitDatabase();
	}

	int HandleServerShutdown()
	{
		if (DNClientProxyHelper::Ptr proxy = GetClientProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;
			proxy->SetRegistEvent(nullptr);

			// web use clientMsg
			proxy->MsgMapClear();
		}

		if (DNWebProxyHelper::Ptr proxy = GetWebProxy())
		{
			if(proxy->service)
			{
				*(proxy->service) = {};
			}
		}

		return true;
	}

};
