module;
#include "StdMacro.h"
export module DNClientProxyHelper;

export import DNClientProxy;
import DNTask;
import Macro;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export class DNClientProxyHelper : public DNClientProxy
{
private:
	DNClientProxyHelper() = delete;
public:
	// regist to controlserver
	RegistState& RegistState() { return eRegistState; }

	void SetRegistEvent(function<void()> event);
	// task
	DNTask<Message*>* GetMsg(uint32_t msgId);
	bool AddMsg(uint32_t msgId, DNTask<Message*>* task, uint32_t breakTime = 10000);
	void DelMsg(uint32_t msgId);

	void MsgMapClear();
};

void DNClientProxyHelper::SetRegistEvent(function<void()> event)
{
	pRegistEvent = event;
}

DNTask<Message*>* DNClientProxyHelper::GetMsg(uint32_t msgId)
{
	shared_lock<shared_mutex> lock(oMsgMutex);
	if (mMsgList.contains(msgId))
	{
		return mMsgList[msgId];
	}
	return nullptr;
}

bool DNClientProxyHelper::AddMsg(uint32_t msgId, DNTask<Message*>* task, uint32_t breakTime)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	mMsgList.emplace(msgId, task);
	// timeout
	if (breakTime > 0)
	{
		task->TimerId() = TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, CheckMessageTimeoutTimer, this, breakTime, msgId);
	}
	return true;
}

void DNClientProxyHelper::DelMsg(uint32_t msgId)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	if (mMsgList.contains(msgId))
	{
		if (DNTask<Message*>* task = mMsgList[msgId])
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

void DNClientProxyHelper::MsgMapClear()
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	for (auto& [k, v] : mMsgList)
	{
		v->CallResume();
	}
	mMsgList.clear();
}
