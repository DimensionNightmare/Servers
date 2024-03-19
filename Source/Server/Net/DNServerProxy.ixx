module;
#include "hv/TcpServer.h"
#include "google/protobuf/message.h"

#include <functional> 
#include <shared_mutex>
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
	void AddTimerRecord(size_t timerId, unsigned int id);

public:
	// cant init in tcpclient this class
	EventLoopThreadPtr pLoop;

protected:
	// only oddnumber
	atomic<unsigned int> iMsgId;
	// unordered_
	map<unsigned int, DNTask<Message*>* > mMsgList;
	//
	map<uint64_t, unsigned int > mMapTimer;

	shared_mutex oMsgMutex;
	shared_mutex oTimerMutex;
};



DNServerProxy::DNServerProxy()
{
	iMsgId = ATOMIC_VAR_INIT(0);
	mMsgList.clear();
	mMapTimer.clear();
}

void DNServerProxy::MessageTimeoutTimer(uint64_t timerID)
{
	unsigned int id = -1;
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
			DNTask<Message *>* task = mMsgList[id];
			mMsgList.erase(id);
			task->SetFlag(DNTaskFlag::Timeout);
			task->CallResume();
		}
	}
		
}

void DNServerProxy::ChannelTimeoutTimer(uint64_t timerID)
{
	unsigned int id = -1;
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
			}
		}
	}

}

void DNServerProxy::AddTimerRecord(size_t timerId, unsigned int id)
{
	unique_lock<shared_mutex> ulock(oTimerMutex);
	mMapTimer.emplace(timerId, id);
}
