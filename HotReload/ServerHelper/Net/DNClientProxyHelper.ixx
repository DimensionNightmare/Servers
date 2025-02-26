module;
#include "StdMacro.h"
export module DNClientProxyHelper;

import DNClientProxy;
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
	EMRegistState& EMRegistState() { return eRegistState; }

	void SetRegistEvent(std::function<void()> event)
	{
		pRegistEvent = event;
	}
	
	// task
	DNTask<Message*>* GetMsg(uint32_t msgId)
	{
		std::shared_lock<std::shared_mutex> lock(oMsgMutex);
		if (mMsgList.contains(msgId))
		{
			return mMsgList[msgId];
		}
		return nullptr;
	}

	bool AddMsg(uint32_t msgId, DNTask<Message*>* task, uint32_t breakTime = 10000)
	{
		std::unique_lock<std::shared_mutex> ulock(oMsgMutex);
		mMsgList.emplace(msgId, task);
		// timeout
		if (breakTime > 0)
		{
			task->TimerId() = TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, CheckMessageTimeoutTimer, this, breakTime, msgId);
		}
		return true;
	}

	void DelMsg(uint32_t msgId)
	{
		std::unique_lock<std::shared_mutex> ulock(oMsgMutex);
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

	void MsgMapClear()
	{
		std::unique_lock<std::shared_mutex> ulock(oMsgMutex);
		for (auto& [k, v] : mMsgList)
		{
			v->CallResume();
		}
		mMsgList.clear();
	}
};
