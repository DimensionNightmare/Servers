module;
#include <functional>
#include <shared_mutex>
#include <cstdint>

#include "StdMacro.h"
export module DNServerProxyHelper;

export import DNServerProxy;
import DNTask;
import Macro;
import ThirdParty.PbGen;

export class DNServerProxyHelper : public DNServerProxy
{
private:
	DNServerProxyHelper() = delete;;

public:
	uint32_t GetMsgId() { return ++iMsgId; }

	bool AddMsg(uint32_t msgId, DNTask<Message*>* task, uint32_t breakTime = 10000);
	DNTask<Message*>* GetMsg(uint32_t msgId);
	void DelMsg(uint32_t msgId);

	void MsgMapClear();
};

bool DNServerProxyHelper::AddMsg(uint32_t msgId, DNTask<Message*>* task, uint32_t breakTime)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	mMsgList.emplace(msgId, task);
	if (breakTime > 0)
	{
		task->TimerId() = TICK_MAINSPACE_SIGN_FUNCTION(DNServerProxy, CheckMessageTimeoutTimer, this, breakTime, msgId);
	}
	return true;
}

DNTask<Message*>* DNServerProxyHelper::GetMsg(uint32_t msgId)
{
	shared_lock<shared_mutex> lock(oMsgMutex);
	if (mMsgList.contains(msgId))
	{
		return mMsgList[msgId];
	}
	return nullptr;
}

void DNServerProxyHelper::DelMsg(uint32_t msgId)
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

void DNServerProxyHelper::MsgMapClear()
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	for (auto& [k, v] : mMsgList)
	{
		v->CallResume();
	}
	mMsgList.clear();
}
