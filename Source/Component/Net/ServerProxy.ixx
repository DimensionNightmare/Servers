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
import ThirdParty.Protobuf;

export class ServerProxy : public Component, public hv::TcpServerTmpl<SocketChannel>
{
protected:
	friend class System;
	friend class UniversalMemoryPool;
	ServerProxy(System::WPtr system):Component(system)
		,TcpServerTmpl(nullptr)
		,CheckMessageTimeoutTimer(this)
		,InitConnectedChannel(this)
	{
		eComponentType = EMComponentType::ServerProxy;
		
		pTimer = GetOwner()->GetWorld()->GetSystemW<Timer>(EMSystemType::Timer);
	}

public:
	using Ptr = std::shared_ptr<ServerProxy>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<ServerProxy>;

	virtual ~ServerProxy()
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
					LoggerPrint::Log(GetWorld(), EL10nCode_SrvNeedIPPort);
					// return false;
					return false;
				}

				inport = stoi(*param);
			}
		}
		

		int listenfd = createsocket(inport, "0.0.0.0");
		if (listenfd < 0)
		{
			LoggerPrint::Log(GetWorld(), EL10nCode_CreateSocket);
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
				LoggerPrint::Log(GetWorld(), EL10nCode_GetSocketName);
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

		LoggerPrint::Log(GetWorld(), EL10nCode_SrvListenOn, port, listenfd);

		GetOwner()->GetWorld()->AddEvent(EMEventType::ServerStart, GetSelfW<ServerProxy>(), &ServerProxy::Start);

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

	virtual void Dispose() override
	{
		Component::Dispose();
		
		End();

		mMsgList.clear();
		mMapTimer.clear();
	}

public: // dll override

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
		FunctionContainer<&ServerProxy::ChannelTimeoutTimer> funcProxy(this);
		
		size_t timerId = GetTimer()->SetTimeout(5000, funcProxy);
		AddTimerRecord(timerId, channel->id());
	}

	Timer::Ptr GetTimer(){ return pTimer.expired() ? nullptr : pTimer.lock(); }

private:

	void _InitConnectedChannel(SocketChannel::CVPtr channel)
	{
		// if not regist
		CheckChannelByTimer(channel);
		// if not recive data

		// channel->setReadTimeout(15000);
	}
	
	uint64_t _CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId)
	{
		FunctionContainer<&ServerProxy::MessageTimeoutTimer> funcProxy(this);
		
		uint64_t timerId = GetTimer()->SetTimeout(breakTime, funcProxy);
		std::unique_lock ulock(oTimerMutex);
		mMapTimer[timerId] = msgId;
		return timerId;
	}
public:
	// cant init in tcpclient this class
	FunctionContainer<&ServerProxy::_InitConnectedChannel> InitConnectedChannel;

	FunctionContainer<&ServerProxy::_CheckMessageTimeoutTimer> CheckMessageTimeoutTimer;

protected:
	// only oddnumber
	std::atomic<uint32_t> iMsgId;
	// unordered_
	std::unordered_map<uint32_t, Task<Message*>* > mMsgList;
	//
	std::unordered_map<uint64_t, uint32_t > mMapTimer;

	std::shared_mutex oMsgMutex;

	std::shared_mutex oTimerMutex;

	Timer::WPtr pTimer;
};
