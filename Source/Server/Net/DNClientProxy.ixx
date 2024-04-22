module;
#include <functional> 
#include <shared_mutex>
#include "hv/TcpClient.h"
#include "hv/EventLoopThread.h"
#include "google/protobuf/message.h"

#include "StdAfx.h"
#include "Server/S_Common.pb.h"
export module DNClientProxy;

import DNTask;
import MessagePack;

using namespace std;
using namespace google::protobuf;
using namespace hv;
using namespace GMsg::S_Common;

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

	void AddTimerRecord(size_t timerId, uint32_t id);

	void TickHeartbeat();

public:
	// cant init in tcpclient this class
	EventLoopThreadPtr pLoop;

protected: // dll proxy
	// only oddnumber
	atomic<uint32_t> iMsgId;
	// unordered_
	map<uint32_t, DNTask<Message>* > mMsgList;
	//
	map<uint64_t, uint32_t > mMapTimer;
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
			DNPrint(ErrCode_NotCallbackEvent, LoggerLevel::Error, nullptr);
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

void DNClientProxy::AddTimerRecord(size_t timerId, uint32_t id)
{
	unique_lock<shared_mutex> ulock(oTimerMutex);
	mMapTimer.emplace(timerId, id);
}

void DNClientProxy::TickHeartbeat()
{
	static COM_RetHeartbeat requset;
	requset.Clear();
	int timespan = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
	requset.set_timespan(timespan);

	static string binData;
	binData.clear();
	binData.resize(requset.ByteSize());
	requset.SerializeToArray(binData.data(), binData.size());

	MessagePack(0, MsgDeal::Ret, requset.GetDescriptor()->full_name().c_str(), binData);
	
	send(binData);
}