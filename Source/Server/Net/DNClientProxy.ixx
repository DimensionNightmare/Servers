module;
#include "StdAfx.h"
#include "hv/TcpClient.h"
#include "google/protobuf/message.h"

#include <functional> 
#include <shared_mutex>
export module DNClientProxy;

import DNTask;

using namespace std;
using namespace google::protobuf;

export class DNClientProxy : public hv::TcpClient
{
public:
	DNClientProxy();
	~DNClientProxy();

public: // dll override
	void TickRegistEvent(size_t timerID);

protected: // dll proxy
	// only oddnumber
	atomic<unsigned int> iMsgId;
	// unordered_
	map<unsigned int, DNTask<Message*>* > mMsgList;
	// status
	bool bIsRegisted;

	function<void()> pRegistEvent;

	hv::Channel::Status eState;

	shared_mutex oMsgMutex;

	bool bIsRegisting;
};



DNClientProxy::DNClientProxy()
{
	iMsgId = ATOMIC_VAR_INIT(0);
	mMsgList.clear();
	bIsRegisted = false;
	pRegistEvent = nullptr;
	eState = hv::Channel::Status::CLOSED;
	bIsRegisting = false;
}

DNClientProxy::~DNClientProxy()
{
	for(auto& [k,v] : mMsgList)
	{
		v->CallResume();
	}
		
	mMsgList.clear();
}

void DNClientProxy::TickRegistEvent(size_t timerID)
{
	if(bIsRegisting)
	{
		return;
	}
	
	if (channel->isConnected() && !bIsRegisted) 
	{
		if(pRegistEvent)
		{
			pRegistEvent();
		}
		else
		{
			DNPrint(16, LoggerLevel::Error, nullptr);
		}
	} 
	else 
	{
		loop()->killTimer(timerID);
	}
}
