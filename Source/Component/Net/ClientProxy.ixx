export module ClientProxy;

import MessagePack;
import ECSW;
import Logger;
import std.compat;
import Task;
import Server;
import ThirdParty.Libhv;

#define NABS(n) ((n) < 0 ? (n) : -(n))

export enum class EMRegistState : uint8_t
{
	None,
	Registing,
	Registed,
};

export class ClientProxy : public Component, public hv::TcpClientTmpl<SocketChannel>
{
protected:
	friend class System;
	friend class UniversalMemoryPool;
	ClientProxy(System::WPtr system):Component(system),TcpClientTmpl(nullptr)
	{
		eComponentType = EMComponentType::ClientProxy;

		// pLoop = std::make_unique<EventLoopThread>();
		pLoop = MemPool->Allocate<EventLoopThread>();
		

		pLogger = GetOwner()->GetWorld()->GetSystemW<LoggerPrint>(EMSystemType::LoggerPrint);

		pInitConnectedChannel = std::bind(&ClientProxy::InitConnectedChannel, this, std::placeholders::_1);
		pCheckMessageTimeoutTimer = std::bind(&ClientProxy::CheckMessageTimeoutTimer, this, std::placeholders::_1, std::placeholders::_2);
		pRedirectClient = std::bind(&ClientProxy::RedirectClient, this, std::placeholders::_1, std::placeholders::_2);
	}
public:

	using Ptr = std::shared_ptr<ClientProxy>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<ClientProxy>;

	virtual ~ClientProxy()
	{
		
	}

	virtual void Dispose() override
	{
		mMsgList.clear();
		mMapTimer.clear();

		End();
		Component::Dispose();
		pLoop = nullptr;
	}

	bool Awake() override
	{
		World::CVPtr world = GetOwner()->GetWorld();
		std::string* ctlPort = world->LaunchParam("ctlPort");
		std::string* ctlIp = world->LaunchParam("ctlIp");
		if (!ctlPort || !ctlIp)
		{
			return false;
		}
		
		createsocket(stoi(*ctlPort), ctlIp->c_str());
		
		reconn_setting_t reconn;
		reconn.min_delay = 1000;
		reconn.max_delay = 10000;
		reconn.delay_policy = 2;
		setReconnect(&reconn);

		unpack_setting_t setting;
		setting.mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
		setting.length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
		setting.body_offset = MessagePacket::PackLenth;
		setting.length_field_bytes = 1;
		setting.length_field_offset = 0;
		setUnpack(&setting);

		GetOwner()->GetWorld()->AddEvent(EMEventType::ServerStart, GetSelfW<ClientProxy>(), &ClientProxy::Start);

		return true;
	}

	void Start()
	{
		pLoop->start();

		// first split self to base pointer
		auto base_ptr = static_cast<TcpClientTmpl<SocketChannel>*>(this);
		// then cast to template<>
		Libhv::Run(base_ptr);
	}

	void End()
	{
		pLoop->stop(true);
		stop(true);
	}


	LoggerPrint::Ptr GetLogger(){ return pLogger.expired() ? nullptr : pLogger.lock(); }

public: // dll override

	void TickRegistEvent(size_t timerID)
	{
		if (eRegistState == EMRegistState::Registing)
		{
			return;
		}

		if (channel->isConnected() && eRegistState != EMRegistState::Registed)
		{
			if (pRegistEvent)
			{
				pRegistEvent(GetOwner<Server>());
			}
			else
			{
				// LoggerPrint()(EL10nCode_NotCallbackEvent);
			}
		}
		else
		{
			Timer()->killTimer(timerID);
		}
	}

	void MessageTimeoutTimer(uint64_t timerID)
	{
		uint32_t msgId = -1;
		{
			if (!mMapTimer.contains(timerID))
			{
				return;
			}

			std::unique_lock ulock(oTimerMutex);
			msgId = mMapTimer[timerID];
			mMapTimer.erase(timerID);
		}

		{
			if (mMsgList.contains(msgId))
			{
				std::unique_lock ulock(oMsgMutex);
				Task<Message*>* task = mMsgList[msgId];
				mMsgList.erase(msgId);
				task->SetFlag(EMTaskFlag::Timeout);
				task->CallResume();
			}
		}
	}

	uint64_t CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId)
	{
		uint64_t timerId = Timer()->setTimeout(breakTime, std::bind(&ClientProxy::MessageTimeoutTimer, this, std::placeholders::_1));
		std::unique_lock ulock(oTimerMutex);
		mMapTimer[timerId] = msgId;
		return timerId;
	}

	const auto& Timer() { return pLoop->loop(); }

	void AddTimerRecord(size_t timerId, uint32_t id)
	{
		std::unique_lock ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}

	void TickHeartbeat()
	{
		// GMsg::COM_RetHeartbeat request;
		// request.Clear();
		// int64_t timespan = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		// request.set_timespan(timespan);

		// std::string binData;
		// request.SerializeToString(&binData);

		// MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, GetChannel());
	}

	void InitConnectedChannel(SocketChannel::CVPtr chanhel)
	{
		// chanhel->setHeartbeat(4000, std::bind(&ClientProxy::TickHeartbeat, this));
		// channel->setWriteTimeout(12000);
		if (eRegistState == EMRegistState::None)
		{
			Timer()->setInterval(1000, std::bind(&ClientProxy::TickRegistEvent, this, std::placeholders::_1));
		}
	}

	void RedirectClient(uint16_t port, const std::string& ip)
	{
		GetLogger()->Record(ELogLevel_Debug, "reclient to {}:{}", ip, port);

		eRegistState = EMRegistState::None;
		closesocket();
		Timer()->setTimeout(500, [this, port, ip](uint64_t)
		{
			createsocket(port, ip.c_str());

			// first split self to base pointer
			auto base_ptr = static_cast<TcpClientTmpl<SocketChannel>*>(this);
			// then cast to template<>
			Libhv::Run(base_ptr);
		});
	}

public:

	std::function<void(SocketChannel::CVPtr)> pInitConnectedChannel;

	std::function<uint64_t(uint32_t,uint32_t)> pCheckMessageTimeoutTimer;

	std::function<void(uint16_t,const std::string&)> pRedirectClient;

protected: // dll proxy

	std::shared_ptr<EventLoopThread> pLoop;

	// only oddnumber
	std::atomic<uint32_t> iMsgId;

	// unordered_
	std::unordered_map<uint32_t, Task<Message*>* > mMsgList;

	//
	std::unordered_map<uint64_t, uint32_t > mMapTimer;

	// status
	EMRegistState eRegistState = EMRegistState::None;
	
	// callback regist to server‘s servertype
	uint8_t iRegistType = 0;

	std::function<void(Server::CVPtr)> pRegistEvent;

	std::shared_mutex oMsgMutex;

	std::shared_mutex oTimerMutex;

	LoggerPrint::WPtr pLogger;
};
