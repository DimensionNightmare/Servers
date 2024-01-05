module;
#include "hv/TcpClient.h"

#include <functional> 
export module DNClientProxy;

import DNTask;

using namespace std;

export class DNClientProxy : public hv::TcpClient
{
public:
	DNClientProxy();
	~DNClientProxy();

public: // dll override

protected: // dll proxy
	// only oddnumber
	unsigned char iMsgId;
	// unordered_
	map<unsigned int, DNTask<void*>* > mMsgList;
	// status
	bool bIsRegisted;

	function<void()> pRegistEvent;
};

module:private;

DNClientProxy::DNClientProxy()
{
	iMsgId = 0;
	mMsgList.clear();
	bIsRegisted = false;
	pRegistEvent = nullptr;
}

DNClientProxy::~DNClientProxy()
{
	for(auto& [k,v] : mMsgList)
	{
		v->Destroy();
	}
		
	mMsgList.clear();
}