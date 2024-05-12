module;
#include <functional> 
#include <shared_mutex>
#include <cstdint>
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
using namespace GMsg;

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
	~DNClientProxy();

	void Start();

	void End();

public: // dll override
	void TickRegistEvent(size_t timerID);
	void StartRegist();

	void MessageTimeoutTimer(uint64_t timerID);
	uint32_t CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId);

	const EventLoopPtr& Timer(){return pLoop->loop();}

	void AddTimerRecord(size_t timerId, uint32_t id);

	void TickHeartbeat();

	void RedirectClient(uint16_t port, const char* ip);

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

	shared_mutex oMsgMutex;
	shared_mutex oTimerMutex;
};



DNClientProxy::DNClientProxy()
{
	iMsgId = 0;
	mMsgList.clear();
	eRegistState = RegistState::None;
	pRegistEvent = nullptr;
}

DNClientProxy::~DNClientProxy()
{
	mMsgList.clear();
	mMapTimer.clear();
}

void DNClientProxy::Start()
{
	pLoop->start();
	start();
}

void DNClientProxy::End()
{	
	pLoop->stop(true);
	stop();
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

void DNClientProxy::StartRegist()
{
	DNPrint(0, LoggerLevel::Debug, "StartRegist tick");
	Timer()->setInterval(1000, std::bind(&DNClientProxy::TickRegistEvent, this, placeholders::_1));
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

uint32_t DNClientProxy::CheckMessageTimeoutTimer(uint32_t breakTime, uint32_t msgId)
{
	uint32_t timerId = Timer()->setTimeout(breakTime, std::bind(&DNClientProxy::MessageTimeoutTimer, this, placeholders::_1));
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
	COM_RetHeartbeat requset;
	requset.Clear();
	int timespan = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
	requset.set_timespan(timespan);

	string binData;
	binData.clear();
	binData.resize(requset.ByteSizeLong());
	requset.SerializeToArray(binData.data(), binData.size());

	MessagePack(0, MsgDeal::Ret, requset.GetDescriptor()->full_name().c_str(), binData);
	
	send(binData);
}

void DNClientProxy::RedirectClient(uint16_t port, const char *ip)
{
	eRegistState = RegistState::None;
	
	createsocket(port, ip);
	start();
}
