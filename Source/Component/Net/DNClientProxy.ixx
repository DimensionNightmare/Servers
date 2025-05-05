module;
export module DNClientProxy;

import DNTask;
import FuncHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import MessagePack;
import ECSW;

#define NABS(n) ((n) < 0 ? (n) : -(n))

export enum class EMRegistState : uint8_t
{
	None,
	Registing,
	Registed,
};

export class DNClientProxy : public Component, public hv::TcpClient
{
protected:
	friend class System;
	DNClientProxy(System::Ptr system):Component(system)
	{
		eComponentType = EMComponentType::DNClientProxy;

		pLoop = std::make_shared<hv::EventLoopThread>();
	}
public:

	~DNClientProxy()
	{
		pLoop = nullptr;
		mMsgList.clear();
		mMapTimer.clear();
	}

	bool Awake() override
	{
		World::Ptr world = GetOwner()->GetWorld();
		std::string* ctlPort = world->LaunchParam("ctlPort");
		std::string* ctlIp = world->LaunchParam("ctlIp");
		if (!ctlPort || !ctlIp)
		{
			return false;
		}
		
		createsocket(stoi(*ctlPort), ctlIp->c_str());
		
		hv::reconn_setting_t reconn;
		reconn.min_delay = 1000;
		reconn.max_delay = 10000;
		reconn.delay_policy = 2;
		setReconnect(&reconn);

		hv::unpack_setting_t setting;
		setting.mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
		setting.length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
		setting.body_offset = MessagePacket::PackLenth;
		setting.length_field_bytes = 1;
		setting.length_field_offset = 0;
		setUnpack(&setting);

		return true;
	}

	void Start()
	{
		pLoop->start();
		Libhv::Run(this);
	}

	void End()
	{
		pLoop->stop(true);
		stop(true);
	}
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
				pRegistEvent();
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

	const hv::EventLoopPtr& Timer() { return pLoop->loop(); }

	void AddTimerRecord(size_t timerId, uint32_t id)
	{
		std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}

	void TickHeartbeat()
	{
		GMsg::COM_RetHeartbeat request;
		request.Clear();
		int64_t timespan = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		request.set_timespan(timespan);

		std::string binData;
		request.SerializeToString(&binData);

		MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, GetChannel());
	}

	void InitConnectedChannel(const hv::SocketChannelPtr& chanhel)
	{
		// chanhel->setHeartbeat(4000, std::bind(&DNClientProxy::TickHeartbeat, this));
		// channel->setWriteTimeout(12000);
		if (eRegistState == EMRegistState::None)
		{
			Timer()->setInterval(1000, std::bind(&DNClientProxy::TickRegistEvent, this, std::placeholders::_1));
		}
	}

	void RedirectClient(uint16_t port, std::string ip)
	{
		SPidLogger.Record(ELogLevel_Debug, "reclient to {}:{}", ip, port);

		eRegistState = EMRegistState::None;
		closesocket();
		Timer()->setTimeout(500, [=](uint64_t)
		{
			createsocket(port, ip.c_str());
			// start();
			Libhv::Run(this);
		});
	}

	bool AddMsg(uint32_t msgId, DNTask<Message*>* task, uint32_t breakTime)
	{
		std::unique_lock<std::shared_mutex> ulock(oMsgMutex);
		mMsgList.emplace(msgId, task);
		// timeout
		if (breakTime > 0)
		{
			task->TimerId() = CheckMessageTimeoutTimer(breakTime, msgId);
		}
		return true;
	}

	uint8_t& RegistType() { return iRegistType; }

	uint32_t GetMsgId() { return ++iMsgId; }

	const hv::SocketChannelPtr& GetChannel() { return channel; }

protected: // dll proxy

	std::shared_ptr<hv::EventLoopThread> pLoop;

	// only oddnumber
	std::atomic<uint32_t> iMsgId;

	// unordered_
	std::unordered_map<uint32_t, DNTask<Message*>* > mMsgList;

	//
	std::unordered_map<uint64_t, uint32_t > mMapTimer;

	// status
	EMRegistState eRegistState = EMRegistState::None;

	uint8_t iRegistType = 0;

	std::function<void()> pRegistEvent;

	std::shared_mutex oMsgMutex;

	std::shared_mutex oTimerMutex;
};
