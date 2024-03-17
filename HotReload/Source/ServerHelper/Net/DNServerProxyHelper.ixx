module;
#include "hv/Channel.h"
#include "google/protobuf/message.h"

#include <functional>
#include <shared_mutex>
export module DNServerProxyHelper;

import DNServerProxy;

using namespace std;
using namespace google::protobuf;

export class DNServerProxyHelper : public DNServerProxy
{
private:
	DNServerProxyHelper(){};

public:
	unsigned int GetMsgId() { return ++iMsgId; }

	bool AddMsg(unsigned int msgId, DNTask<Message*>* msg, int breakTime = 10000);
	DNTask<Message*>* GetMsg(unsigned int msgId);
	void DelMsg(unsigned int msgId);

	void TickHeartbeat(hio_t *io);
};

bool DNServerProxyHelper::AddMsg(unsigned int msgId, DNTask<Message *> *msg, int breakTime)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	mMsgList.emplace(msgId, msg);
	if(breakTime > 0)
	{
		loop()->setTimeout(breakTime, [this,msgId](uint64_t timerID)
		{
			if(DNTask<Message *>* task = GetMsg(msgId))
			{
				DelMsg(msgId);
				task->CallResume();
			}
		});
	}
	return true;
}

DNTask<Message *> *DNServerProxyHelper::GetMsg(unsigned int msgId)
{;
	shared_lock<shared_mutex> lock(oMsgMutex);
	if(mMsgList.contains(msgId))
	{
		return mMsgList[msgId];
	}
	return nullptr;
}

void DNServerProxyHelper::DelMsg(unsigned int msgId)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	mMsgList.erase(msgId);
}

void DNServerProxyHelper::TickHeartbeat(hio_t* io)
{
	hv::SocketChannel* channel = (hv::SocketChannel*)hio_context(io);
	//Regist?
	if(!channel->context())
	{
		channel->close(true);
		return;
	}
};