module;
#include <functional>
#include <shared_mutex>
#include <cstdint>
#include "hv/TcpClient.h"

#include "StdAfx.h"
#include "Server/S_Common.pb.h"
export module DNClientProxyHelper;

export import DNClientProxy;
import DNTask;

using namespace std;
using namespace hv;
using namespace google::protobuf;

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
		task->TimerId() = DNClientProxy::CheckMessageTimeoutTimer(breakTime, msgId);
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
