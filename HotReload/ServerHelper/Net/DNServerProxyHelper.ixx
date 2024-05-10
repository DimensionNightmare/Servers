module;
#include <functional>
#include <shared_mutex>
#include <cstdint>
#include "hv/Channel.h"
#include "google/protobuf/message.h"
export module DNServerProxyHelper;

export import DNServerProxy;
import DNTask;

using namespace std;
using namespace google::protobuf;

export class DNServerProxyHelper : public DNServerProxy
{
private:
	DNServerProxyHelper(){};

public:
	uint32_t GetMsgId() { return ++iMsgId; }

	bool AddMsg(uint32_t msgId, DNTask<Message>* task, uint32_t breakTime = 10000);
	DNTask<Message>* GetMsg(uint32_t msgId);
	void DelMsg(uint32_t msgId);
};

bool DNServerProxyHelper::AddMsg(uint32_t msgId, DNTask<Message> *task, uint32_t breakTime)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	mMsgList.emplace(msgId, task);
	if(breakTime > 0)
	{
		task->TimerId() = CheckMessageTimeoutTimer(breakTime, msgId);
	}
	return true;
}

DNTask<Message> *DNServerProxyHelper::GetMsg(uint32_t msgId)
{
	shared_lock<shared_mutex> lock(oMsgMutex);
	if(mMsgList.contains(msgId))
	{
		return mMsgList[msgId];
	}
	return nullptr;
}

void DNServerProxyHelper::DelMsg(uint32_t msgId)
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
