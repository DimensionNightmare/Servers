module;
export module DNClientProxyHelper;

import DNClientProxy;
import DNTask;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

#define FUNCPLACE(func) #func, func

export class DNClientProxyHelper : public DNClientProxy
{

private:

	DNClientProxyHelper() = delete;
	~DNClientProxyHelper() = default;

	DNClientProxyHelper(const DNClientProxyHelper&) = delete;
	void operator=(const DNClientProxyHelper&) = delete;

	DNClientProxyHelper(DNClientProxyHelper&&) = delete;
	DNClientProxyHelper& operator=(DNClientProxyHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<DNClientProxyHelper>;

	// regist to controlserver
	EMRegistState GetRegistState() { return eRegistState; }
	void SetRegistState(EMRegistState state) { eRegistState = state; }

	void SetRegistEvent(std::function<void(const DNServer::Ptr& server)> event)
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
			task->TimerId() = TickMainSpaceDll(this, FUNCPLACE(&DNClientProxy::CheckMessageTimeoutTimer),  breakTime, msgId);
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
