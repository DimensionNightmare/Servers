module;
#include "StdAfx.h"
#include "hv/TcpClient.h"
#include "hv/EventLoopThread.h"
#include "google/protobuf/message.h"

#include <functional> 
#include <shared_mutex>
export module DNClientProxy;

import DNTask;

using namespace std;
using namespace google::protobuf;
using namespace hv;

export class DNClientProxy : public TcpClientTmpl<SocketChannel>
{
public:
	DNClientProxy();
	~DNClientProxy();

public: // dll override
	void TickRegistEvent(size_t timerID);

	void MessageTimeoutTimer(uint64_t timerID);

	const EventLoopPtr& Timer(){return pLoop->loop();}

	void AddTimerRecord(size_t timerId, unsigned int id);

public:
	// cant init in tcpclient this class
	EventLoopThreadPtr pLoop;

protected: // dll proxy
	// only oddnumber
	atomic<unsigned int> iMsgId;
	// unordered_
	map<unsigned int, DNTask<Message>* > mMsgList;
	//
	map<uint64_t, unsigned int > mMapTimer;
	// status
	bool bIsRegisted;

	function<void()> pRegistEvent;

	Channel::Status eState;

	shared_mutex oMsgMutex;
	shared_mutex oTimerMutex;

	bool bIsRegisting;
};



DNClientProxy::DNClientProxy()
{
	iMsgId = ATOMIC_VAR_INIT(0);
	mMsgList.clear();
	bIsRegisted = false;
	pRegistEvent = nullptr;
	eState = Channel::Status::CLOSED;
	bIsRegisting = false;
}

DNClientProxy::~DNClientProxy()
{
	for(auto& [k,v] : mMsgList)
	{
		v->CallResume();
	}
		
	mMsgList.clear();
}

void DNClientProxy::TickRegistEvent(size_t timerID)
{
	if(bIsRegisting)
	{
		return;
	}
	
	if (channel->isConnected() && !bIsRegisted) 
	{
		if(pRegistEvent)
		{
			pRegistEvent();
		}
		else
		{
			DNPrint(16, LoggerLevel::Error, nullptr);
		}
	} 
	else 
	{
		Timer()->killTimer(timerID);
	}
}

void DNClientProxy::MessageTimeoutTimer(uint64_t timerID)
{
	unsigned int msgId = -1;
	{
		if(!mMapTimer.contains(timerID))
		{
			return;
		}

		unique_lock<shared_mutex> ulock(oTimerMutex);
		msgId = mMapTimer[timerID];
		mMapTimer.erase(timerID);
	}

	{
		if(mMsgList.contains(msgId))
		{
			unique_lock<shared_mutex> ulock(oMsgMutex);
			DNTask<Message>* task = mMsgList[msgId];
			mMsgList.erase(msgId);
			task->SetFlag(DNTaskFlag::Timeout);
			task->CallResume();
		}
	}
}

void DNClientProxy::AddTimerRecord(size_t timerId, unsigned int id)
{
	unique_lock<shared_mutex> ulock(oTimerMutex);
	mMapTimer.emplace(timerId, id);
}