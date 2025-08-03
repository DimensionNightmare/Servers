export module ServerProxy;

import MessagePack;
import ThirdParty.Platform;
import ECSW;
import Logger;
import std.compat;
import Task;
import Server;
import ThirdParty.Libhv;

export class ServerProxy : public Component, public hv::TcpServerTmpl<SocketChannel>
{
protected:
	friend class System;
	friend class UniversalMemoryPool;
	ServerProxy(System::WPtr system):Component(system),TcpServerTmpl(nullptr)
	{
		eComponentType = EMComponentType::ServerProxy;

		// pLoop = std::make_unique<EventLoopThread>();
		pLoop = MemPool->Allocate<EventLoopThread>();
		
		pLogger = GetOwner()->GetWorld()->GetSystemW<LoggerPrint>(EMSystemType::LoggerPrint);
	}

public:
	using Ptr = std::shared_ptr<ServerProxy>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<ServerProxy>;

	~ServerProxy()
	{
		
	}

	bool Awake() override
	{
		int16_t inport = 0;

		Server::CVPtr dnServer = GetOwner<Server>();

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

		GetOwner()->GetWorld()->AddEvent(EMEventType::ServerStart, GetSelfW<ServerProxy>(), &ServerProxy::Start);

		return true;
	}

	void Start()
	{
		pLoop->start();

		// first split self to base pointer
		auto base_ptr = static_cast<TcpServerTmpl<SocketChannel>*>(this);
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

	void InitConnectedChannel(SocketChannel::CVPtr channel)
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

			std::unique_lock ulock(oTimerMutex);
			id = mMapTimer[timerID];
			mMapTimer.erase(timerID);
		}

		{
			if (mMsgList.contains(id))
			{
				std::unique_lock ulock(oMsgMutex);
				Task<Message*>* task = mMsgList[id];
				mMsgList.erase(id);
				if(task)
				{
					task->SetFlag(EMTaskFlag::Timeout);
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

			std::unique_lock ulock(oTimerMutex);
			id = mMapTimer[timerID];
			mMapTimer.erase(timerID);
		}

		{
			if (SocketChannel::CVPtr channel = getChannelById(id))
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
		std::unique_lock ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}

	void CheckChannelByTimer(SocketChannel::CVPtr channel)
	{
		size_t timerId = Timer()->setTimeout(5000, std::bind(&ServerProxy::ChannelTimeoutTimer, this, std::placeholders::_1));
		AddTimerRecord(timerId, channel->id());
	}
	
	uint64_t CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId)
	{
		uint64_t timerId = Timer()->setTimeout(breakTime, std::bind(&ServerProxy::MessageTimeoutTimer, this, std::placeholders::_1));
		std::unique_lock ulock(oTimerMutex);
		mMapTimer[timerId] = msgId;
		return timerId;
	}
public:
	// cant init in tcpclient this class
	std::shared_ptr<EventLoopThread> pLoop;

protected:
	// only oddnumber
	std::atomic<uint32_t> iMsgId;
	// unordered_
	std::unordered_map<uint32_t, Task<Message*>* > mMsgList;
	//
	std::unordered_map<uint64_t, uint32_t > mMapTimer;

	std::shared_mutex oMsgMutex;

	std::shared_mutex oTimerMutex;

	LoggerPrint::WPtr pLogger;
};
