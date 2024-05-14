module;
#include <cstdint>
#include <functional> 
#include <shared_mutex>
#include "hv/TcpServer.h"
#include "google/protobuf/message.h"

#include "StdAfx.h"
export module DNServerProxy;

import DNTask;

using namespace std;
using namespace google::protobuf;
using namespace hv;

export class DNServerProxy : public TcpServerTmpl<SocketChannel>
{
public:
	DNServerProxy();
	~DNServerProxy();

	void Start();

	void End();

public: // dll override
	void MessageTimeoutTimer(uint64_t timerID);
	void ChannelTimeoutTimer(uint64_t timerID);

	const EventLoopPtr& Timer(){return pLoop->loop();}
	void AddTimerRecord(size_t timerId, uint32_t id);

	void CheckChannelByTimer(const SocketChannelPtr & channel);
	uint32_t CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId);

public:
	// cant init in tcpclient this class
	EventLoopThreadPtr pLoop;

protected:
	// only oddnumber
	atomic<uint32_t> iMsgId;
	// unordered_
	map<uint32_t, DNTask<Message>* > mMsgList;
	//
	map<uint64_t, uint32_t > mMapTimer;

	shared_mutex oMsgMutex;
	shared_mutex oTimerMutex;
};



DNServerProxy::DNServerProxy()
{
	iMsgId = 0;
	mMsgList.clear();
	mMapTimer.clear();
}

DNServerProxy::~DNServerProxy()
{
	mMsgList.clear();
	mMapTimer.clear();
}

void DNServerProxy::Start()
{
	pLoop->start();
	start();
}

void DNServerProxy::End()
{
	pLoop->stop(true);
	stop(true);
}

void DNServerProxy::MessageTimeoutTimer(uint64_t timerID)
{
	uint32_t id = -1;
	{
		if(!mMapTimer.contains(timerID))
		{
			return;
		}

		unique_lock<shared_mutex> ulock(oTimerMutex);
		id = mMapTimer[timerID];
		mMapTimer.erase(timerID);
	}

	{
		if(mMsgList.contains(id))
		{
			unique_lock<shared_mutex> ulock(oMsgMutex);
			DNTask<Message>* task = mMsgList[id];
			mMsgList.erase(id);
			task->SetFlag(DNTaskFlag::Timeout);
			task->CallResume();
		}
	}
		
}

void DNServerProxy::ChannelTimeoutTimer(uint64_t timerID)
{
	uint32_t id = -1;
	{
		if(!mMapTimer.contains(timerID))
		{
			return;
		}

		unique_lock<shared_mutex> ulock(oTimerMutex);
		id = mMapTimer[timerID];
		mMapTimer.erase(timerID);
	}

	{
		if(TSocketChannelPtr channel = getChannelById(id))
		{
			if(!channel->context())
			{
				channel->close();
				DNPrint(0, LoggerLevel::Debug, "ChannelTimeoutTimer server destory entity\n");
			}
		}
	}

}

void DNServerProxy::AddTimerRecord(size_t timerId, uint32_t id)
{
	unique_lock<shared_mutex> ulock(oTimerMutex);
	mMapTimer.emplace(timerId, id);
}

void DNServerProxy::CheckChannelByTimer(const SocketChannelPtr &channel)
{
	size_t timerId = Timer()->setTimeout(5000, std::bind(&DNServerProxy::ChannelTimeoutTimer, this, placeholders::_1));
	AddTimerRecord(timerId, channel->id());
}

uint32_t DNServerProxy::CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId)
{
	uint32_t timerId = Timer()->setTimeout(breakTime, std::bind(&DNServerProxy::MessageTimeoutTimer, this, placeholders::_1));
	mMapTimer[timerId] = msgId;
	return timerId;
}
