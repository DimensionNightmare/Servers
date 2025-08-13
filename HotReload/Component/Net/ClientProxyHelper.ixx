export module ClientProxyHelper;

import ClientProxy;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

export class ClientProxyHelper : public ClientProxy
{

private:

	ClientProxyHelper() = delete;
	~ClientProxyHelper() = default;

	ClientProxyHelper(const ClientProxyHelper&) = delete;
	void operator=(const ClientProxyHelper&) = delete;

	ClientProxyHelper(ClientProxyHelper&&) = delete;
	ClientProxyHelper& operator=(ClientProxyHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<ClientProxyHelper>;
	using CVPtr = const Ptr&;

	EMRegistState GetRegistState() { return eRegistState; }
	void SetRegistState(EMRegistState state) { eRegistState = state; }

	uint8_t RegistType() { return iRegistType; }
	void SetRegistType(uint8_t type) { iRegistType = type; }

	void SetRegistEvent(std::function<void(Server::CVPtr)> event)
	{
		pRegistEvent = event;
	}
	
	// task
	Task<Message*>* GetMsg(uint32_t msgId)
	{
		std::shared_lock lock(oMsgMutex);
		if (mMsgList.contains(msgId))
		{
			return mMsgList[msgId];
		}
		return nullptr;
	}

	bool AddMsg(uint32_t msgId, Task<Message*>* task, uint32_t breakTime = 10000)
	{
		std::unique_lock ulock(oMsgMutex);
		mMsgList.emplace(msgId, task);
		// timeout
		if (breakTime > 0)
		{
			task->TimerId() = pCheckMessageTimeoutTimer(breakTime, msgId);
		}
		return true;
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

	uint32_t GetMsgId() { return ++iMsgId; }
	
	SocketChannel::CVPtr GetChannel() { return channel; }
	
};
