module;
#include "hv/EventLoop.h"
#include "hv/hsocket.h"
#include "StdMacro.h"
export module DNClientProxy;

import DNTask;
import FuncHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export enum class RegistState : uint8_t
{
	None,
	Registing,
	Registed,
};

class TcpClientTmplTemp : public EventLoopThread, public TcpClientEventLoopTmpl<SocketChannel>
{
public:
	TcpClientTmplTemp(EventLoopPtr loop = NULL)
		: EventLoopThread(loop)
		, TcpClientEventLoopTmpl<SocketChannel>(EventLoopThread::loop())
		, is_loop_owner(loop == NULL)
	{
	}
	virtual ~TcpClientTmplTemp()
	{
		stop(true);
	}

	const EventLoopPtr& loop()
	{
		return EventLoopThread::loop();
	}

	// start thread-safe
	void start(bool wait_threads_started = true)
	{
		if (isRunning())
		{
			TcpClientEventLoopTmpl<SocketChannel>::start();
		}
		else
		{
			EventLoopThread::start(wait_threads_started, [this]()
				{
					TcpClientTmplTemp::startConnect();
					return 0;
				});
		}
	}

	// stop thread-safe
	void stop(bool wait_threads_stopped = true)
	{
		TcpClientEventLoopTmpl<SocketChannel>::closesocket();
		if (is_loop_owner)
		{
			EventLoopThread::stop(wait_threads_stopped);
		}
	}

private:
	bool is_loop_owner;
};

export class DNClientProxy : public TcpClientTmplTemp
{
public:
	DNClientProxy();
	~DNClientProxy();

	void Init();

	void Start();

	void End();

public: // dll override
	void TickRegistEvent(size_t timerID);

	void MessageTimeoutTimer(uint64_t timerID);
	uint64_t CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId);

	const EventLoopPtr& Timer() { return pLoop->loop(); }

	void AddTimerRecord(size_t timerId, uint32_t id);

	void TickHeartbeat();

	void InitConnectedChannel(const SocketChannelPtr& chanhel);

	void RedirectClient(uint16_t port, string ip);

	bool AddMsg(uint32_t msgId, DNTask<Message*>* task, uint32_t breakTime);

	uint8_t& RegistType() { return iRegistType; }

	uint32_t GetMsgId() { return ++iMsgId; }

	const SocketChannelPtr& GetChannel() { return channel; }

protected: // dll proxy
	shared_ptr<EventLoopThread> pLoop;
	// only oddnumber
	atomic<uint32_t> iMsgId;
	// unordered_
	unordered_map<uint32_t, DNTask<Message*>* > mMsgList;
	//
	unordered_map<uint64_t, uint32_t > mMapTimer;
	// status
	RegistState eRegistState = RegistState::None;
	uint8_t iRegistType = 0;

	function<void()> pRegistEvent;

	shared_mutex oMsgMutex;
	shared_mutex oTimerMutex;
};

extern "C"
{
	REGIST_MAINSPACE_SIGN_FUNCTION(DNClientProxy, InitConnectedChannel);
	REGIST_MAINSPACE_SIGN_FUNCTION(DNClientProxy, CheckMessageTimeoutTimer);
	REGIST_MAINSPACE_SIGN_FUNCTION(DNClientProxy, RedirectClient);
}

DNClientProxy::DNClientProxy()
{
	pLoop = make_shared<EventLoopThread>();
}

DNClientProxy::~DNClientProxy()
{
	pLoop = nullptr;
	mMsgList.clear();
	mMapTimer.clear();
}

void DNClientProxy::Init()
{
	shared_ptr<reconn_setting_t> reconn = make_shared<reconn_setting_t>();
	reconn->min_delay = 1000;
	reconn->max_delay = 10000;
	reconn->delay_policy = 2;
	setReconnect(reconn.get());

	shared_ptr<unpack_setting_t> setting = make_shared<unpack_setting_t>();
	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = MessagePacket::PackLenth;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;
	setUnpack(setting.get());

}

void DNClientProxy::Start()
{
	pLoop->start();
	start();
}

void DNClientProxy::End()
{
	pLoop->stop(true);
	stop(true);
}

void DNClientProxy::TickRegistEvent(size_t timerID)
{
	if (eRegistState == RegistState::Registing)
	{
		return;
	}

	if (channel->isConnected() && eRegistState != RegistState::Registed)
	{
		if (pRegistEvent)
		{
			pRegistEvent();
		}
		else
		{
			DNPrint(ErrCode::ErrCode_NotCallbackEvent, LoggerLevel::Error, nullptr);
		}
	}
	else
	{
		Timer()->killTimer(timerID);
	}
}

void DNClientProxy::MessageTimeoutTimer(uint64_t timerID)
{
	uint32_t msgId = -1;
	{
		if (!mMapTimer.contains(timerID))
		{
			return;
		}

		unique_lock<shared_mutex> ulock(oTimerMutex);
		msgId = mMapTimer[timerID];
		mMapTimer.erase(timerID);
	}

	{
		if (mMsgList.contains(msgId))
		{
			unique_lock<shared_mutex> ulock(oMsgMutex);
			DNTask<Message*>* task = mMsgList[msgId];
			mMsgList.erase(msgId);
			task->SetFlag(DNTaskFlag::Timeout);
			task->CallResume();
		}
	}
}

uint64_t DNClientProxy::CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId)
{
	uint64_t timerId = Timer()->setTimeout(breakTime, std::bind(&DNClientProxy::MessageTimeoutTimer, this, placeholders::_1));
	unique_lock<shared_mutex> ulock(oTimerMutex);
	mMapTimer[timerId] = msgId;
	return timerId;
}

void DNClientProxy::AddTimerRecord(size_t timerId, uint32_t id)
{
	unique_lock<shared_mutex> ulock(oTimerMutex);
	mMapTimer.emplace(timerId, id);
}

void DNClientProxy::TickHeartbeat()
{
	COM_RetHeartbeat request;
	request.Clear();
	int timespan = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
	request.set_timespan(timespan);

	string binData;
	request.SerializeToString(&binData);

	MessagePackAndSend(0, MsgDeal::Ret, request.GetDescriptor()->full_name().c_str(), binData, GetChannel());
}

void DNClientProxy::InitConnectedChannel(const SocketChannelPtr& chanhel)
{
	// chanhel->setHeartbeat(4000, std::bind(&DNClientProxy::TickHeartbeat, this));
	// channel->setWriteTimeout(12000);
	if (eRegistState == RegistState::None)
	{
		Timer()->setInterval(1000, std::bind(&DNClientProxy::TickRegistEvent, this, placeholders::_1));
	}
}

void DNClientProxy::RedirectClient(uint16_t port, string ip)
{
	DNPrint(0, LoggerLevel::Debug, "reclient to %s:%u", ip.c_str(), port);

	eRegistState = RegistState::None;
	closesocket();
	Timer()->setTimeout(500, [=](uint64_t)
		{
			createsocket(port, ip.c_str());
			start();
		});
}

bool DNClientProxy::AddMsg(uint32_t msgId, DNTask<Message*>* task, uint32_t breakTime)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	mMsgList.emplace(msgId, task);
	// timeout
	if (breakTime > 0)
	{
		task->TimerId() = CheckMessageTimeoutTimer(breakTime, msgId);
	}
	return true;
}
