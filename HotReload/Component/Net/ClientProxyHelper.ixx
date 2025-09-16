export module ClientProxyHelper;

import ClientProxy;
import FuncUtils;

export class ClientProxyHelper : public Helper<ClientProxyHelper, ClientProxy>
{

private:

	ClientProxyHelper() = delete;
	~ClientProxyHelper() = default;

public:

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
			task->TimerId() = CheckMessageTimeoutTimer(breakTime, msgId);
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
					GetTimer()->KillTimer(timerId);
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
