module;
#include "hv/TcpServer.h"
#include "google/protobuf/message.h"

#include <functional> 
#include <shared_mutex>
export module DNServerProxy;

import DNTask;

using namespace std;
using namespace google::protobuf;

export class DNServerProxy : public hv::TcpServer
{
public:
	DNServerProxy();
	~DNServerProxy(){};

public: // dll override
	void MessageTimeoutTimer(uint64_t timerID);

protected:
	// only oddnumber
	atomic<unsigned int> iMsgId;
	// unordered_
	map<unsigned int, DNTask<Message*>* > mMsgList;
	//
	map<uint64_t, unsigned int > mMsgListTimer;

	shared_mutex oMsgMutex;
	shared_mutex oMsgTimerMutex;
};



DNServerProxy::DNServerProxy()
{
	iMsgId = ATOMIC_VAR_INIT(0);
	mMsgList.clear();
}

void DNServerProxy::MessageTimeoutTimer(uint64_t timerID)
{
	unsigned int msgId = -1;
	{
		if(!mMsgListTimer.contains(timerID))
		{
			return;
		}

		unique_lock<shared_mutex> ulock(oMsgTimerMutex);
		msgId = mMsgListTimer[timerID];
		mMsgListTimer.erase(timerID);
	}

	{
		if(mMsgList.contains(msgId))
		{
			unique_lock<shared_mutex> ulock(oMsgMutex);
			DNTask<Message *>* task = mMsgList[msgId];
			mMsgList.erase(msgId);
			task->SetFlag(DNTaskFlag::Timeout);
			task->CallResume();
		}
	}
		
}
