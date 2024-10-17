module;
#include "StdMacro.h"
export module DNServerProxy;

import DNTask;
import MessagePack;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

class TcpServerTmplTemp : public EventLoopThread, public TcpServerEventLoopTmpl<SocketChannel>
{

public:

	TcpServerTmplTemp(EventLoopPtr loop = nullptr)
		: EventLoopThread(loop)
		, TcpServerEventLoopTmpl<SocketChannel>(EventLoopThread::loop())
		, is_loop_owner(loop == nullptr)
	{
	}

	virtual ~TcpServerTmplTemp()
	{
		stop(true);
	}

	EventLoopPtr loop(int idx = -1)
	{
		return TcpServerEventLoopTmpl<SocketChannel>::loop(idx);
	}

	// start thread-safe
	void start(bool wait_threads_started = true)
	{
		TcpServerEventLoopTmpl<SocketChannel>::start(wait_threads_started);
		EventLoopThread::start(wait_threads_started);
	}

	// stop thread-safe
	void stop(bool wait_threads_stopped = true)
	{
		if (is_loop_owner)
		{
			EventLoopThread::stop(wait_threads_stopped);
		}
		TcpServerEventLoopTmpl<SocketChannel>::stop(wait_threads_stopped);
	}
private:

	bool is_loop_owner;
};

export class DNServerProxy : public TcpServerTmplTemp
{

public:

	DNServerProxy()
{
	pLoop = make_shared<EventLoopThread>();
}

	~DNServerProxy()
{
	pLoop = nullptr;
	mMsgList.clear();
	mMapTimer.clear();
}

	void Init()
{
	// if not set port mean need get port by self 
	if (!port && listenfd > 0)
	{
			sockaddr_in addr;
			int addrLen = sizeof(addr);
			if (LibhvExport::getsockname(listenfd, reinterpret_cast<struct sockaddr*>(&addr), &addrLen) < 0)
		{
			DNPrint(ErrCode::ErrCode_GetSocketName, LoggerLevel::Error, nullptr);
			return;
		}

			port = LibhvExport::ntohs(addr.sin_port);
	}

	shared_ptr<unpack_setting_t> setting = make_shared<unpack_setting_t>();
	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = MessagePacket::PackLenth;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;
	setUnpack(setting.get());
	setThreadNum(4);
}

	void Start()
{
	pLoop->start();
	start();
}

	void End()
{
	pLoop->stop(true);
	stop(true);
}

public: // dll override

	void InitConnectedChannel(const SocketChannelPtr& channel)
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

		unique_lock<shared_mutex> ulock(oTimerMutex);
		id = mMapTimer[timerID];
		mMapTimer.erase(timerID);
	}

	{
		if (mMsgList.contains(id))
		{
			unique_lock<shared_mutex> ulock(oMsgMutex);
			DNTask<Message*>* task = mMsgList[id];
			mMsgList.erase(id);
			task->SetFlag(DNTaskFlag::Timeout);
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

		unique_lock<shared_mutex> ulock(oTimerMutex);
		id = mMapTimer[timerID];
		mMapTimer.erase(timerID);
	}

	{
		if (TSocketChannelPtr channel = getChannelById(id))
		{
			if (!channel->context())
			{
				channel->close();
				DNPrint(0, LoggerLevel::Debug, "ChannelTimeoutTimer server destory entity\n");
			}
		}
	}

}

	const EventLoopPtr& Timer() { return pLoop->loop(); }

	void AddTimerRecord(size_t timerId, uint32_t id)
{
	unique_lock<shared_mutex> ulock(oTimerMutex);
	mMapTimer.emplace(timerId, id);
}

	void CheckChannelByTimer(SocketChannelPtr channel)
{
	size_t timerId = Timer()->setTimeout(5000, std::bind(&DNServerProxy::ChannelTimeoutTimer, this, placeholders::_1));
	AddTimerRecord(timerId, channel->id());
}
	uint64_t CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId)
{
	uint64_t timerId = Timer()->setTimeout(breakTime, std::bind(&DNServerProxy::MessageTimeoutTimer, this, placeholders::_1));
	unique_lock<shared_mutex> ulock(oTimerMutex);
	mMapTimer[timerId] = msgId;
	return timerId;
	}
public:
	// cant init in tcpclient this class
	shared_ptr<EventLoopThread> pLoop;

protected:
	// only oddnumber
	atomic<uint32_t> iMsgId;
	// unordered_
	unordered_map<uint32_t, DNTask<Message*>* > mMsgList;
	//
	unordered_map<uint64_t, uint32_t > mMapTimer;

	shared_mutex oMsgMutex;
	
	shared_mutex oTimerMutex;
};

extern "C"
{
	REGIST_MAINSPACE_SIGN_FUNCTION(DNServerProxy, InitConnectedChannel);
	REGIST_MAINSPACE_SIGN_FUNCTION(DNServerProxy, CheckMessageTimeoutTimer);
}
