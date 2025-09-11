export module ServerProxyHelper;

import ServerProxy;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

export class ServerProxyHelper : public ServerProxy
{
	
private:

	ServerProxyHelper() = delete;
	~ServerProxyHelper() = default;

	ServerProxyHelper(const ServerProxyHelper&) = delete;
	void operator=(const ServerProxyHelper&) = delete;

	ServerProxyHelper(ServerProxyHelper&&) = delete;
	ServerProxyHelper& operator=(ServerProxyHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<ServerProxyHelper>;
	using CVPtr = const Ptr&;

	uint32_t GetMsgId() { return ++iMsgId; }

	bool AddMsg(uint32_t msgId, Task<Message*>* task, uint32_t breakTime = 10000)
	{
		std::unique_lock ulock(oMsgMutex);
		mMsgList.emplace(msgId, task);
		if (breakTime > 0)
		{
			task->TimerId() = CheckMessageTimeoutTimer(breakTime, msgId);
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
		std::unique_lock ulock(oMsgMutex);
		for (auto& [k, v] : mMsgList)
		{
			v->CallResume();
		}
		mMsgList.clear();
	}
};
