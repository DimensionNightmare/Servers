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
	~DNServerProxy(){};

public: // dll override
	void MessageTimeoutTimer(uint64_t timerID);
	void ChannelTimeoutTimer(uint64_t timerID);

	const EventLoopPtr& Timer(){return pLoop->loop();}
	void AddTimerRecord(size_t timerId, uint32_t id);

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
