module;
export module DNClientProxy;

import MessagePack;
import ECSW;
import Logger;
import std.compat;
import DNTask;
import DNServer;
import ThirdParty.Libhv;

#define NABS(n) ((n) < 0 ? (n) : -(n))

export enum class EMRegistState : uint8_t
{
	None,
	Registing,
	Registed,
};

export class DNClientProxy : public Component, public hv::TcpClientTmpl<DNSocketChannel>
{
protected:
	friend class System;
	DNClientProxy(System::WPtr system):Component(system),TcpClientTmpl(nullptr)
	{
		eComponentType = EMComponentType::DNClientProxy;

		pLoop = std::make_unique<EventLoopThread>();

		pLogger = GetOwner()->GetWorld()->GetSystemW<LoggerPrint>(EMSystemType::LoggerPrint);
	}
public:

	using Ptr = std::shared_ptr<DNClientProxy>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<DNClientProxy>;

	~DNClientProxy()
	{
		pLoop = nullptr;
		mMsgList.clear();
		mMapTimer.clear();
	}

	virtual void Dispose() override
	{
		End();

		Component::Dispose();
	}

	bool Awake() override
	{
		World::CVPtr pWorld = GetOwner()->GetWorld();
		std::string* ctlPort = pWorld->LaunchParam("ctlPort");
		std::string* ctlIp = pWorld->LaunchParam("ctlIp");
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

		GetOwner()->GetWorld()->AddEvent(EMEventType::ServerStart, GetSelfW<DNClientProxy>(), &DNClientProxy::Start);

		return true;
	}

	void Start()
	{
		pLoop->start();

		// first split self to base pointer
		auto base_ptr = static_cast<TcpClientTmpl<DNSocketChannel>*>(this);
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
				pRegistEvent(GetOwner<DNServer>());
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

			std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
			msgId = mMapTimer[timerID];
			mMapTimer.erase(timerID);
		}

		{
			if (mMsgList.contains(msgId))
			{
				std::unique_lock<std::shared_mutex> ulock(oMsgMutex);
				DNTask<Message*>* task = mMsgList[msgId];
				mMsgList.erase(msgId);
				task->SetFlag(EMDNTaskFlag::Timeout);
				task->CallResume();
			}
		}
	}

	uint64_t CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId)
	{
		uint64_t timerId = Timer()->setTimeout(breakTime, std::bind(&DNClientProxy::MessageTimeoutTimer, this, std::placeholders::_1));
		std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
		mMapTimer[timerId] = msgId;
		return timerId;
	}

	const auto& Timer() { return pLoop->loop(); }

	void AddTimerRecord(size_t timerId, uint32_t id)
	{
		std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
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

	void InitConnectedChannel(DNSocketChannel::CVPtr chanhel)
	{
		// chanhel->setHeartbeat(4000, std::bind(&DNClientProxy::TickHeartbeat, this));
		// channel->setWriteTimeout(12000);
		if (eRegistState == EMRegistState::None)
		{
			Timer()->setInterval(1000, std::bind(&DNClientProxy::TickRegistEvent, this, std::placeholders::_1));
		}
	}

	void RedirectClient(uint16_t port, const std::string& ip)
	{
		GetLogger()->Record(ELogLevel_Debug, "reclient to {}:{}", ip, port);

		eRegistState = EMRegistState::None;
		closesocket();
		Timer()->setTimeout(500, [=](uint64_t)
		{
			createsocket(port, ip.c_str());

			// first split self to base pointer
			auto base_ptr = static_cast<TcpClientTmpl<DNSocketChannel>*>(this);
			// then cast to template<>
			Libhv::Run(base_ptr);
		});
	}

protected: // dll proxy

	std::unique_ptr<EventLoopThread> pLoop;

	// only oddnumber
	std::atomic<uint32_t> iMsgId;

	// unordered_
	std::unordered_map<uint32_t, DNTask<Message*>* > mMsgList;

	//
	std::unordered_map<uint64_t, uint32_t > mMapTimer;

	// status
	EMRegistState eRegistState = EMRegistState::None;
	
	// callback regist to server‘s servertype
	uint8_t iRegistType = 0;

	std::function<void(DNServer::CVPtr)> pRegistEvent;

	std::shared_mutex oMsgMutex;

	std::shared_mutex oTimerMutex;

	LoggerPrint::WPtr pLogger;
};
