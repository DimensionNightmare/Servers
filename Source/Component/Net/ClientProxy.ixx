export module ClientProxy;

import MessagePack;
import ECSW;
import Logger;
import std.compat;
import Task;
import Server;
import ThirdParty.Libhv;
import FuncUtils;
import Timer;

export enum class EMRegistState : uint8_t
{
	None = 0,
	Registing,
	Registed,
};

export class ClientProxy : public Component, public hv::TcpClientTmpl<SocketChannel>
{
protected:

	friend class UniversalMemoryPool;
	ClientProxy(System::WPtr system):Component(system)
		,TcpClientTmpl(nullptr)
		,CheckMessageTimeoutTimer(this)
		,InitConnectedChannel(this)
		,RedirectClient(this)
	{
		eComponentType = EMComponentType::ClientProxy;
		
		pTimer = GetWorld()->GetSystemW<Timer>(EMSystemType::Timer);
	}
public:

	using Ptr = std::shared_ptr<ClientProxy>;
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
	}

	bool Awake() override
	{
		World::Ptr world = GetWorld();
		std::string* ctlPort = world->GetParam("ctlPort");
		std::string* ctlIp = world->GetParam("ctlIp");
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

		GetWorld()->AddEvent<&ClientProxy::Start>(EMEventType::ServerStart, GetSelfW<ClientProxy>());
		GetWorld()->AddEvent<&ClientProxy::End>(EMEventType::ServerStop, GetSelfW<ClientProxy>());

		return true;
	}

	void Start()
	{
		// first split self to base pointer
		auto base_ptr = static_cast<TcpClientTmpl<SocketChannel>*>(this);
		// then cast to template<>
		Libhv::Run(base_ptr);
	}

	void End()
	{
		stop(true);
	}

public: // dll override

	void TickRegistEvent(size_t timerID)
	{
		if(timerID != mRegisterTimeId)
		{
			GetTimer()->KillTimer(timerID);
			return;
		}

		if (eRegistState == EMRegistState::Registing)
		{
			return;
		}

		if (channel->isConnected() && eRegistState != EMRegistState::Registed)
		{
			GetWorld()->Broadcast(EMEventType::ClientProxyRegist);
		}
		else
		{
			mRegisterTimeId = 0;
		}
	}

	void MessageTimeoutTimer(size_t timerID)
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
				MsgTask* task = mMsgList[msgId];
				mMsgList.erase(msgId);
				task->SetFlag(EMTaskFlag::Timeout);
				task->Resume();
			}
		}
	}

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

		// AddMsg(EMMsgDeal::Ret, &request);
	}

	Timer::Ptr GetTimer(){ return pTimer.expired() ? nullptr : pTimer.lock(); }

protected:

	size_t _CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId)
	{
		EventContainer<&ClientProxy::MessageTimeoutTimer> funcProxy(this);

		size_t timerId = GetTimer()->SetTimeout(breakTime, funcProxy);
		std::unique_lock ulock(oTimerMutex);
		mMapTimer[timerId] = msgId;
		return timerId;
	}

	void _InitConnectedChannel(const SocketChannel::Ptr& chanhel)
	{
		// chanhel->setHeartbeat(4000, std::bind(&ClientProxy::TickHeartbeat, this));
		// channel->setWriteTimeout(12000);

		EventContainer<&ClientProxy::TickRegistEvent> funcProxy(this);

		if (eRegistState == EMRegistState::None)
		{
			mRegisterTimeId = GetTimer()->SetInterval(1000, funcProxy);
		}
	}

	void _RedirectClient(uint16_t port, const std::string& ip)
	{
		LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "reclient to {}:{}", ip, port);

		// 在注册中时
		mRegisterTimeId = 0;

		eRegistState = EMRegistState::None;
		closesocket();
		GetTimer()->SetTimeout(500, [this, port, ip](size_t)
		{
			createsocket(port, ip.c_str());

			// first split self to base pointer
			auto base_ptr = static_cast<TcpClientTmpl<SocketChannel>*>(this);
			// then cast to template<>
			Libhv::Run(base_ptr);
		});
	}

public:

	EventContainer<&ClientProxy::_InitConnectedChannel> InitConnectedChannel;

	EventContainer<&ClientProxy::_CheckMessageTimeoutTimer> CheckMessageTimeoutTimer;

	EventContainer<&ClientProxy::_RedirectClient> RedirectClient;

protected: // dll proxy

	// only oddnumber
	std::atomic<uint32_t> iMsgId;

	// unordered_
	std::unordered_map<uint32_t, MsgTask* > mMsgList;

	//
	std::unordered_map<size_t, uint32_t > mMapTimer;

	// status
	EMRegistState eRegistState = EMRegistState::None;

	size_t mRegisterTimeId = 0;
	
	// callback regist to server‘s servertype
	uint8_t iRegistType = 0;

	std::shared_mutex oMsgMutex;

	std::shared_mutex oTimerMutex;

	Timer::WPtr pTimer;
};
