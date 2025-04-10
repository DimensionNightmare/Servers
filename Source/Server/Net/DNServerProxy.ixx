module;
#include "StdMacro.h"
export module DNServerProxy;

import DNTask;
import MessagePack;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export class DNServerProxy : public TcpServer
{

public:

	DNServerProxy()
	{
		pLoop = std::make_shared<EventLoopThread>();
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
			if (getsockname(listenfd, reinterpret_cast<struct sockaddr*>(&addr), &addrLen) < 0)
			{
				DNPrintCode(EL10nCode_GetSocketName);
				return;
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
	}

	void Start()
	{
		pLoop->start();
		// start();
		HVRun(this);
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
					DNPrint(ELogLevel_Debug, "ChannelTimeoutTimer server destory entity\n");
				}
			}
		}

	}

	const EventLoopPtr& Timer() { return pLoop->loop(); }

	void AddTimerRecord(size_t timerId, uint32_t id)
	{
		std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}

	void CheckChannelByTimer(SocketChannelPtr channel)
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
	std::shared_ptr<EventLoopThread> pLoop;

protected:
	// only oddnumber
	std::atomic<uint32_t> iMsgId;
	// unordered_
	std::unordered_map<uint32_t, DNTask<Message*>* > mMsgList;
	//
	std::unordered_map<uint64_t, uint32_t > mMapTimer;

	std::shared_mutex oMsgMutex;

	std::shared_mutex oTimerMutex;
};
