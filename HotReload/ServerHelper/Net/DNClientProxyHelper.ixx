module;
#include "StdAfx.h"
#include "S_Common.pb.h"
#include "hv/TcpClient.h"

#include <functional>
#include <shared_mutex>
export module DNClientProxyHelper;

import DNClientProxy;
import MessagePack;

using namespace std;
using namespace hv;
using namespace google::protobuf;

export enum class ProxyStatus : char
{
	None,
	Open,
	Close,
};

export class DNClientProxyHelper : public DNClientProxy
{
private:
	DNClientProxyHelper();
public:

	unsigned int GetMsgId() { return ++iMsgId; }

	// regist to controlserver
	RegistState& RegistState(){return eRegistState;}

	void SetRegistEvent(function<void()> event);
	// client status
	ProxyStatus UpdateClientState(Channel::Status state);
	void ServerDisconnect();
	// task
	DNTask<Message>* GetMsg(unsigned int msgId);
	bool AddMsg(unsigned int msgId, DNTask<Message>* task, unsigned int breakTime = 10000);
	void DelMsg(unsigned int msgId);
	// heartbeat
	void TickHeartbeat();
};

void DNClientProxyHelper::SetRegistEvent(function<void()> event)
{
	pRegistEvent = event;
}

ProxyStatus DNClientProxyHelper::UpdateClientState(Channel::Status state)
{
	if(state == eState || (state == Channel::Status::CONNECTING && eState == Channel::Status::CLOSED))
	{
		return ProxyStatus::None;
	}

	eState = state;

	switch (eState)
	{
	case Channel::Status::CONNECTED :
		{
			//maybe sometimes tick (wait fix)
			Timer()->setInterval(1000, std::bind(&DNClientProxy::TickRegistEvent, reinterpret_cast<DNClientProxy*>(this), placeholders::_1));
			return ProxyStatus::Open;
		}

	case Channel::Status::CLOSED :
	case Channel::Status::DISCONNECTED :
		{
			ServerDisconnect();
			return ProxyStatus::Close;
		}
	}

	return ProxyStatus::None;

}

void DNClientProxyHelper::ServerDisconnect()
{
	eRegistState = RegistState::None;

	for(auto& [k,v] : mMsgList)
	{
		v->CallResume();
	}
		
	mMsgList.clear();
}

DNTask<Message> *DNClientProxyHelper::GetMsg(unsigned int msgId)
{
	shared_lock<shared_mutex> lock(oMsgMutex);
	if(mMsgList.contains(msgId))
	{
		return mMsgList[msgId];
	}
	return nullptr;
}

bool DNClientProxyHelper::AddMsg(unsigned int msgId, DNTask<Message> *task, unsigned int breakTime)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	mMsgList.emplace(msgId, task);
	// timeout
	if(breakTime > 0)
	{
		task->TimerId() = Timer()->setTimeout(breakTime, std::bind(&DNClientProxy::MessageTimeoutTimer, reinterpret_cast<DNClientProxy*>(this), placeholders::_1));
		mMapTimer[task->TimerId()] = msgId;
	}
	return true;
}

void DNClientProxyHelper::DelMsg(unsigned int msgId)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	if(mMsgList.contains(msgId))
	{
		if(DNTask<Message> *task = mMsgList[msgId])
		{
			if (size_t timerId = task->TimerId())
			{
				Timer()->killTimer(timerId);
				mMapTimer.erase(timerId);
			}
		}
	}
	mMsgList.erase(msgId);
}

void DNClientProxyHelper::TickHeartbeat()
{
	static GMsg::S_Common::COM_RetHeartbeat requset;
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
