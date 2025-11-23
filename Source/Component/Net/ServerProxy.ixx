export module ServerProxy;

import MessagePack;
import ThirdParty.Platform;
import ECSW;
import Logger;
import std.compat;
import Task;
import Server;
import ThirdParty.Libhv;
import FuncUtils;
import Timer;

export class ServerProxy : public Component, public hv::TcpServerTmpl<SocketChannel>
{
protected:

	friend class UniversalMemoryPool;
	ServerProxy(System::CVPtr system):Component(system)
		,TcpServerTmpl(nullptr)
		,CheckMessageTimeoutTimer(this)
		,InitConnectedChannel(this)
	{
		eComponentType = EMComponentType::ServerProxy;
		
		pTimer = GetWorld()->GetSystem<Timer>(EMSystemType::Timer);
	}

public:
	using Ptr = std::shared_ptr<ServerProxy>;
	using CVPtr = const Ptr&;

	virtual ~ServerProxy()
	{
		
	}

	bool Awake() override
	{
		int16_t inport = 0;

		Server::CVPtr dnServer = GetOwner<Server>();

		World::CVPtr world = GetWorld();

		switch(dnServer->GetServerType())
		{
			case EMServerType::ControlServer:
			case EMServerType::GlobalServer:
			case EMServerType::AuthServer:
			{
				std::string* param = world->GetParam("port");
				if (!param)
				{
					LoggerPrint::Log(world, EL10nCode_SrvNeedIPPort);
					return false;
				}

				inport = stoi(*param);
			}
		}
		

		int listenfd = createsocket(inport, "0.0.0.0");
		if (listenfd < 0)
		{
			LoggerPrint::Log(world, EL10nCode_CreateSocket);
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
				LoggerPrint::Log(world, EL10nCode_GetSocketName);
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
		setThreadNum(4);

		LoggerPrint::Log(world, EL10nCode_SrvListenOn, port, listenfd);

		world->AddEvent<&ServerProxy::Start>(EMEventType::ServerStart, GetSelf<ServerProxy>());
		world->AddEvent<&ServerProxy::End>(EMEventType::ServerStop, GetSelf<ServerProxy>());
		world->AddEvent<&ServerProxy::Pause>(EMEventType::ServerPause, GetSelf<ServerProxy>());
		world->AddEvent<&ServerProxy::Resume>(EMEventType::ServerResume, GetSelf<ServerProxy>());

		return true;
	}

	void Start()
	{
		// first split self to base pointer
		auto base_ptr = static_cast<TcpServerTmpl<SocketChannel>*>(this);
		// then cast to template<>
		Libhv::Run(base_ptr);
	}

	void End()
	{
		stop(true);
	}
	
	void Pause()
	{
		std::unordered_map<long, bool> looped;
		while (const hv::EventLoopPtr& pLoop = loop())
		{
			long id = pLoop->tid();
			if (!looped.count(id))
			{
				pLoop->pause();
				looped[id];
			}
			else
			{
				break;
			}
		};
	}

	void Resume()
	{
		std::unordered_map<long, bool> looped;
		while (const hv::EventLoopPtr& pLoop = loop())
		{
			long id = pLoop->tid();
			if (!looped.count(id))
			{
				pLoop->resume();
				looped[id];
			}
			else
			{
				break;
			}
		};
	}

	virtual void Dispose() override
	{
		Pause();

		End();
		
		mMsgList.clear();
		mMapTimer.clear();
		
		Component::Dispose();
	}

public: 

	void MessageTimeoutTimer(size_t timerID)
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
				MsgTask* task = mMsgList[id];
				mMsgList.erase(id);
				if(task)
				{
					task->SetFlag(EMTaskFlag::Timeout);
					task->Resume();
				}
			}
		}

	}

	void ChannelTimeoutTimer(size_t timerID)
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
				if (!channel->GetEntity<Entity>())
				{
					LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "ChannelTimeoutTimer dnServer destory entity\n");
					channel->close();
				}
			}
		}

	}

	void AddTimerRecord(size_t timerId, uint32_t id)
	{
		std::unique_lock ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}

	void CheckChannelByTimer(SocketChannel::CVPtr channel)
	{
		EventContainer<&ServerProxy::ChannelTimeoutTimer> funcProxy(this);
		
		size_t timerId = GetTimer()->SetTimeout(5000, funcProxy);
		AddTimerRecord(timerId, channel->id());
	}

	Timer::Ptr GetTimer(){ return pTimer.expired() ? nullptr : pTimer.lock(); }

protected:

	void _InitConnectedChannel(SocketChannel::CVPtr channel)
	{
		// if not regist
		CheckChannelByTimer(channel);
		// if not recive data

		// channel->setReadTimeout(15000);
	}
	
	size_t _CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId)
	{
		EventContainer<&ServerProxy::MessageTimeoutTimer> funcProxy(this);
		
		size_t timerId = GetTimer()->SetTimeout(breakTime, funcProxy);
		std::unique_lock ulock(oTimerMutex);
		mMapTimer[timerId] = msgId;
		return timerId;
	}
public:
	// cant init in tcpclient this class
	EventContainer<&ServerProxy::_InitConnectedChannel> InitConnectedChannel;

	EventContainer<&ServerProxy::_CheckMessageTimeoutTimer> CheckMessageTimeoutTimer;

protected:
	// only oddnumber
	std::atomic<uint32_t> iMsgId;
	// unordered_
	std::unordered_map<uint32_t, MsgTask* > mMsgList;
	//
	std::unordered_map<size_t, uint32_t > mMapTimer;

	std::shared_mutex oMsgMutex;

	std::shared_mutex oTimerMutex;

	Timer::WPtr pTimer;
};
