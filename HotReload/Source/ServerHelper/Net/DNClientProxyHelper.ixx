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
	bool IsRegisted(){return bIsRegisted;}
	void SetRegisted(bool isRegisted){bIsRegisted = isRegisted;}
	void SetIsRegisting(bool state) { bIsRegisting = state;}
	void SetRegistEvent(function<void()> event);
	// client status
	ProxyStatus UpdateClientState(Channel::Status state);
	void ServerDisconnect();
	// msg
	bool AddMsg(unsigned int msgId, DNTask<Message*>* msg, int breakTime = 10000);
	DNTask<Message*>* GetMsg(unsigned int msgId);
	void DelMsg(unsigned int msgId);
	// heartbeat
	void TickHeartbeat();
};

#pragma region // ClientReconnectFunc

static std::function<void(const char*, int)> PClientReconnectFunc = nullptr;

export void SetClientReconnectFunc( std::function<void(const char*, int)> func)
{
	PClientReconnectFunc = func;
}

export std::function<void(const char*, int)> GetClientReconnectFunc()
{
	return PClientReconnectFunc;
}

#pragma endregion


void DNClientProxyHelper::SetRegistEvent(function<void()> event)
{
	pRegistEvent = event;
}

ProxyStatus DNClientProxyHelper::UpdateClientState(Channel::Status state)
{
	if(state == eState)
	{
		return ProxyStatus::None;
	}

	eState = state;

	switch (eState)
	{
	case Channel::Status::CONNECTED :
		{
			loop()->setInterval(1000, std::bind(&DNClientProxy::TickRegistEvent, (DNClientProxy*)this, placeholders::_1));
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
	SetRegisted(false);

	for(auto& [k,v] : mMsgList)
	{
		v->CallResume();
	}
		
	mMsgList.clear();
}

bool DNClientProxyHelper::AddMsg(unsigned int msgId, DNTask<Message *> *msg, int breakTime)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	mMsgList.emplace(msgId, msg);
	// timeout
	if(breakTime > 0)
	{
		loop()->setTimeout(breakTime, [this,msgId](uint64_t timerID)
		{
			if(DNTask<Message *>* task = GetMsg(msgId))
			{
				DelMsg(msgId);
				task->SetFlag(DNTaskFlag::Timeout);
				task->CallResume();
			}
		});
	}
	return true;
}

DNTask<Message *> *DNClientProxyHelper::GetMsg(unsigned int msgId)
{;
	shared_lock<shared_mutex> lock(oMsgMutex);
	if(mMsgList.contains(msgId))
	{
		return mMsgList[msgId];
	}
	return nullptr;
}

void DNClientProxyHelper::DelMsg(unsigned int msgId)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
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

	MessagePack(0, MsgDeal::Req, requset.GetDescriptor()->full_name().c_str(), binData);
	
	send(binData);
}
