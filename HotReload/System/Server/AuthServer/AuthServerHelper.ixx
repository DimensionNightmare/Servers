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
	using CVPtr = const Ptr&;

	DNClientProxyHelper::Ptr GetClientProxy()
	{ 
		DNClientProxyHelper::Ptr proxy = GetComponent<DNClientProxyHelper>(EMComponentType::DNClientProxy);
		if(!proxy || proxy->IsDisposed())
		{
			return nullptr;
		}
		return proxy;
	}

	DNWebProxyHelper::Ptr GetWebProxy()
	{ 
		DNWebProxyHelper::Ptr proxy = GetComponent<DNWebProxyHelper>(EMComponentType::DNWebProxy);
		if(!proxy || proxy->IsDisposed())
		{
			return nullptr;
		}
		return proxy;
	}

	RdbProxyHelper::Ptr GetRdbProxy()
	{ 
		RdbProxyHelper::Ptr proxy = GetComponent<RdbProxyHelper>(EMComponentType::RdbProxy);
		if(!proxy || proxy->IsDisposed())
		{
			return nullptr;
		}
		return proxy;
	}

	bool InitDatabase()
	{
		
		if(RdbProxyHelper::CVPtr proxy = GetRdbProxy())
		{
			try
			{
				World::CVPtr pWorld = GetWorld();

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
		
		msgHandle->RegApiHandle(GetSelf<DNServer>());

		if (DNClientProxy::CVPtr proxy = GetComponent<DNClientProxy>(EMComponentType::DNClientProxy))
		{
			proxy->onConnection = [this,msgHandle](DNSocketChannel::CVPtr channel)
				{
					DNClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

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

			proxy->onMessage = [this](DNSocketChannel::CVPtr channel, hv::Buffer* buf)
				{
					DNClientProxyHelper::CVPtr proxyHelper = GetClientProxy();

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
		if (DNClientProxyHelper::CVPtr proxy = GetClientProxy())
		{
			proxy->onConnection = nullptr;
			proxy->onMessage = nullptr;
			proxy->SetRegistEvent(nullptr);

			// web use clientMsg
			proxy->MsgMapClear();
		}

		if (DNWebProxyHelper::CVPtr proxy = GetWebProxy())
		{
			if(proxy->service)
			{
				*(proxy->service) = {};
			}
		}

		return true;
	}

};
