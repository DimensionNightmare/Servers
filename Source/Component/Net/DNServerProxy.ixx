module;
export module DNServerProxy;

import DNTask;
import MessagePack;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ECSW;
import DNServer;

export class DNServerProxy : public Component, public hv::TcpServer
{
protected:
	friend class System;
	DNServerProxy(System::WPtr system):Component(system)
	{
		eComponentType = EMComponentType::DNServerProxy;

		pLoop = std::make_unique<hv::EventLoopThread>();
		pLogger = GetOwner()->GetWorld()->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint);
	}

public:

	~DNServerProxy()
	{
		
	}

	bool Awake() override
	{
		int16_t port = 0;

		switch(GetOwner<DNServer>()->GetServerType())
		{
			case EMServerType::ControlServer:
			case EMServerType::GlobalServer:
			case EMServerType::AuthServer:
			{
				std::string* inport = GetOwner()->GetWorld()->LaunchParam("port");
				if (!inport)
				{
					GetLogger()->Record(EL10nCode_SrvNeedIPPort);
					// return false;
					return false;
				}

				port = stoi(*inport);
			}
		}
		

		int listenfd = createsocket(port, "0.0.0.0");
		if (listenfd < 0)
		{
			GetLogger()->Record(EL10nCode_CreateSocket);
			// return false;
			return false;
		}

		// if not set port mean need get port by self 
		if (!port && listenfd > 0)
		{
			sockaddr_in addr;
			int addrLen = sizeof(addr);
			if (getsockname(listenfd, reinterpret_cast<struct sockaddr*>(&addr), &addrLen) < 0)
			{
				GetLogger()->Record(EL10nCode_GetSocketName);
				return false;
			}

			port = ntohs(addr.sin_port);
		}

		unpack_setting_t setting;
		setting.mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
		setting.length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
		setting.body_offset = MessagePacket::PackLenth;
		setting.length_field_bytes = 1;
		setting.length_field_offset = 0;
		setUnpack(&setting);
		setThreadNum(4);

		GetLogger()->Record(EL10nCode_SrvListenOn, port, listenfd);

		GetOwner()->AddEvent(EMEventType::ServerStart, GetSelfW<DNServerProxy>(), &DNServerProxy::Start);

		return true;
	}

	void Start()
	{
		pLoop->start();
		// start();
		Libhv::Run(this);
	}

	void End()
	{
		pLoop->stop(true);
		stop(true);
	}

	virtual void Dispose() override
	{
		Component::Dispose();

		pLoop = nullptr;
		mMsgList.clear();
		mMapTimer.clear();
	}

	LoggerPrint::Ptr GetLogger(){ return pLogger.lock(); }

public: // dll override

	void InitConnectedChannel(const hv::SocketChannelPtr& channel)
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
				task->SetFlag(EMDNTaskFlag::Timeout);
				task->CallResume();
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
			if (TSocketChannelPtr channel = getChannelById(id))
			{
				if (!channel->context())
				{
					channel->close();
					GetLogger()->Record(ELogLevel_Debug, "ChannelTimeoutTimer server destory entity\n");
				}
			}
		}

	}

	const hv::EventLoopPtr& Timer() { return pLoop->loop(); }

	void AddTimerRecord(size_t timerId, uint32_t id)
	{
		std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}

	void CheckChannelByTimer(hv::SocketChannelPtr channel)
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
	std::unique_ptr<hv::EventLoopThread> pLoop;

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
