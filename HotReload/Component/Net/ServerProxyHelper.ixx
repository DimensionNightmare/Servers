export module ServerProxyHelper;

import ServerProxy;
import FuncUtils;

export class ServerProxyHelper : public Helper<ServerProxyHelper, ServerProxy>
{
	
private:

	ServerProxyHelper() = delete;
	~ServerProxyHelper() = default;

public:

	uint32_t GetMsgId() { return ++iMsgId; }

	bool AddMsg(uint32_t msgId, Task<Message*>* task, uint32_t breakTime = 10000)
	{
		std::unique_lock ulock(oMsgMutex);
		mMsgList.emplace(msgId, task);
		if (breakTime > 0)
		{
			task->SetTimerId(CheckMessageTimeoutTimer(breakTime, msgId));
		}
		return true;
	}

	Task<Message*>* GetMsg(uint32_t msgId)
	{
		std::shared_lock lock(oMsgMutex);
		if (mMsgList.contains(msgId))
		{
			return mMsgList[msgId];
		}
		return nullptr;
	}

	void DelMsg(uint32_t msgId)
	{
		std::unique_lock ulock(oMsgMutex);
		if (mMsgList.contains(msgId))
		{
			if (Task<Message*>* task = mMsgList[msgId])
			{
				if (size_t timerId = task->GetTimerId())
				{
					GetTimer()->KillTimer(timerId);
					mMapTimer.erase(timerId);
				}
			}
		}
		mMsgList.erase(msgId);
	}

	void ClearMsgMap()
	{
		std::unique_lock ulock(oMsgMutex);
		for (auto& [k, v] : mMsgList)
		{
			v->CallResume();
		}
		mMsgList.clear();
	}
};
