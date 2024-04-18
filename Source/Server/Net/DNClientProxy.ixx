module;
#include <functional> 
#include <shared_mutex>
#include "hv/TcpClient.h"
#include "hv/EventLoopThread.h"
#include "google/protobuf/message.h"

#include "StdAfx.h"
export module DNClientProxy;

import DNTask;

using namespace std;
using namespace google::protobuf;
using namespace hv;

export enum class RegistState
{
	None,
	Registing,
	Registed,
};

export class DNClientProxy : public TcpClientTmpl<SocketChannel>
{
public:
	DNClientProxy();
	~DNClientProxy(){};

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
	RegistState eRegistState;

	function<void()> pRegistEvent;

	Channel::Status eState;

	shared_mutex oMsgMutex;
	shared_mutex oTimerMutex;
};



DNClientProxy::DNClientProxy()
{
	iMsgId = ATOMIC_VAR_INIT(0);
	mMsgList.clear();
	eRegistState = RegistState::None;
	pRegistEvent = nullptr;
	eState = Channel::Status::CLOSED;
}

void DNClientProxy::TickRegistEvent(size_t timerID)
{
	if(eRegistState == RegistState::Registing)
	{
		return;
	}
	
	if (channel->isConnected() && eRegistState != RegistState::Registed) 
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