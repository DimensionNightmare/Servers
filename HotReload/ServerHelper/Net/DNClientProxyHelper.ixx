module;
#include <functional>
#include <shared_mutex>
#include "hv/TcpClient.h"

#include "StdAfx.h"
#include "Server/S_Common.pb.h"
export module DNClientProxyHelper;

import DNClientProxy;

using namespace std;
using namespace hv;
using namespace google::protobuf;

export enum class ProxyStatus : uint8_t
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

	uint32_t GetMsgId() { return ++iMsgId; }

	// regist to controlserver
	RegistState& RegistState(){return eRegistState;}

	void SetRegistEvent(function<void()> event);
	// client status
	ProxyStatus UpdateClientState(Channel::Status state);
	void ServerDisconnect();
	// task
	DNTask<Message>* GetMsg(uint32_t msgId);
	bool AddMsg(uint32_t msgId, DNTask<Message>* task, uint32_t breakTime = 10000);
	void DelMsg(uint32_t msgId);
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
			Timer()->setInterval(1000, std::bind(&DNClientProxy::TickRegistEvent, static_cast<DNClientProxy*>(this), placeholders::_1));
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

DNTask<Message> *DNClientProxyHelper::GetMsg(uint32_t msgId)
{
	shared_lock<shared_mutex> lock(oMsgMutex);
	if(mMsgList.contains(msgId))
	{
		return mMsgList[msgId];
	}
	return nullptr;
}

bool DNClientProxyHelper::AddMsg(uint32_t msgId, DNTask<Message> *task, uint32_t breakTime)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	mMsgList.emplace(msgId, task);
	// timeout
	if(breakTime > 0)
	{
		task->TimerId() = Timer()->setTimeout(breakTime, std::bind(&DNClientProxy::MessageTimeoutTimer, static_cast<DNClientProxy*>(this), placeholders::_1));
		mMapTimer[task->TimerId()] = msgId;
	}
	return true;
}

void DNClientProxyHelper::DelMsg(uint32_t msgId)
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
