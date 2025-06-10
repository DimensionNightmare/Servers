module;
export module DNServerProxy;

import MessagePack;
import ThirdParty.Platform;
import ECSW;
import Logger;
import std.compat;
import DNTask;
import DNServer;
import ThirdParty.Libhv;

export class DNServerProxy : public Component, public hv::TcpServerTmpl<DNSocketChannel>
{
protected:
	friend class System;
	DNServerProxy(System::WPtr system):Component(system),TcpServerTmpl(nullptr)
	{
		eComponentType = EMComponentType::DNServerProxy;

		pLoop = std::make_unique<EventLoopThread>();
		pLogger = GetOwner()->GetWorld()->GetSystemW<LoggerPrint>(EMSystemType::LoggerPrint);
	}

public:
	using Ptr = std::shared_ptr<DNServerProxy>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<DNServerProxy>;

	~DNServerProxy()
	{
		
	}

	bool Awake() override
	{
		int16_t inport = 0;

		DNServer::CVPtr dnServer = GetOwner<DNServer>();

		switch(dnServer->GetServerType())
		{
			case EMServerType::ControlServer:
			case EMServerType::GlobalServer:
			case EMServerType::AuthServer:
			{
				std::string* param = GetOwner()->GetWorld()->LaunchParam("port");
				if (!param)
				{
					GetLogger()->Record(EL10nCode_SrvNeedIPPort);
					// return false;
					return false;
				}

				inport = stoi(*param);
			}
		}
		

		int listenfd = createsocket(inport, "0.0.0.0");
		if (listenfd < 0)
		{
			GetLogger()->Record(EL10nCode_CreateSocket);
			// return false;
			return false;
		}

		
		// if not set port mean need get port by self 
		if (inport)
		{
			port = inport;
		}
		else
		{
			Platform::sockaddr_in addr;
			int addrLen = sizeof(addr);
			if (Platform::getsockname(listenfd, reinterpret_cast<struct Platform::sockaddr*>(&addr), &addrLen) < 0)
			{
				GetLogger()->Record(EL10nCode_GetSocketName);
				return false;
			}

			port = Platform::ntohs(addr.sin_port);
		}

		unpack_setting_t setting;
		setting.mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
		setting.length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
		setting.body_offset = MessagePacket::PackLenth;
		setting.length_field_bytes = 1;
		setting.length_field_offset = 0;
		setUnpack(&setting);
		setThreadNum(1);

		GetLogger()->Record(EL10nCode_SrvListenOn, port, listenfd);

		GetOwner()->GetWorld()->AddEvent(EMEventType::ServerStart, GetSelfW<DNServerProxy>(), &DNServerProxy::Start);

		return true;
	}

	void Start()
	{
		pLoop->start();

		// first split self to base pointer
		auto base_ptr = static_cast<TcpServerTmpl<DNSocketChannel>*>(this);
		// then cast to template<>
		Libhv::Run(base_ptr);
	}

	void End()
	{
		pLoop->stop(true);
		stop(true);
	}

	virtual void Dispose() override
	{
		Component::Dispose();
		
		End();

		pLoop = nullptr;
		mMsgList.clear();
		mMapTimer.clear();
	}

	LoggerPrint::Ptr GetLogger(){ return pLogger.expired() ? nullptr : pLogger.lock(); }

public: // dll override

	void InitConnectedChannel(DNSocketChannel::CVPtr channel)
	{
		// if not regist
		CheckChannelByTimer(channel);
		// if not recive data

		// channel->setReadTimeout(15000);
	}

	void MessageTimeoutTimer(uint64_t timerID)
	{
		uint32_t id = -1;
		{
			if (!mMapTimer.contains(timerID))
			{
				return;
			}

			std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
			id = mMapTimer[timerID];
			mMapTimer.erase(timerID);
		}

		{
			if (mMsgList.contains(id))
			{
				std::unique_lock<std::shared_mutex> ulock(oMsgMutex);
				DNTask<Message*>* task = mMsgList[id];
				mMsgList.erase(id);
				if(task)
				{
					task->SetFlag(EMDNTaskFlag::Timeout);
					task->CallResume();
				}
			}
		}

	}

	void ChannelTimeoutTimer(uint64_t timerID)
	{
		uint32_t id = -1;
		{
			if (!mMapTimer.contains(timerID))
			{
				return;
			}

			std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
			id = mMapTimer[timerID];
			mMapTimer.erase(timerID);
		}

		{
			if (DNSocketChannel::CVPtr channel = getChannelById(id))
			{
				if (!channel->contextPtr())
				{
					GetLogger()->Record(ELogLevel_Debug, "ChannelTimeoutTimer dnServer destory entity\n");
					channel->close();
				}
			}
		}

	}

	const auto& Timer() { return pLoop->loop(); }

	void AddTimerRecord(size_t timerId, uint32_t id)
	{
		std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}

	void CheckChannelByTimer(DNSocketChannel::CVPtr channel)
	{
		size_t timerId = Timer()->setTimeout(5000, std::bind(&DNServerProxy::ChannelTimeoutTimer, this, std::placeholders::_1));
		AddTimerRecord(timerId, channel->id());
	}
	
	uint64_t CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId)
	{
		uint64_t timerId = Timer()->setTimeout(breakTime, std::bind(&DNServerProxy::MessageTimeoutTimer, this, std::placeholders::_1));
		std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
		mMapTimer[timerId] = msgId;
		return timerId;
	}
public:
	// cant init in tcpclient this class
	std::unique_ptr<EventLoopThread> pLoop;

protected:
	// only oddnumber
	std::atomic<uint32_t> iMsgId;
	// unordered_
	std::unordered_map<uint32_t, DNTask<Message*>* > mMsgList;
	//
	std::unordered_map<uint64_t, uint32_t > mMapTimer;

	std::shared_mutex oMsgMutex;

	std::shared_mutex oTimerMutex;

	LoggerPrint::WPtr pLogger;
};
