module;
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

	auto& GetMsgMap(){ return mMsgList; }

	bool AddMsg(unsigned int msgId, DNTask<Message*>* msg);
	DNTask<Message*>* GetMsg(unsigned int msgId);
	void DelMsg(unsigned int msgId);
};


module:private;

bool DNServerProxyHelper::AddMsg(unsigned int msgId, DNTask<Message *> *msg)
{
	unique_lock<shared_mutex> ulock(oMsgMutex);
	mMsgList.emplace(msgId, msg);
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