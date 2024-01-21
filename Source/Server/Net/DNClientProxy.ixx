module;
#include "hv/TcpClient.h"

#include <functional> 
#include <shared_mutex>
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
	atomic<unsigned int> iMsgId;
	// unordered_
	map<unsigned int, DNTask<void*>* > mMsgList;
	// status
	bool bIsRegisted;

	function<void()> pRegistEvent;

	hv::Channel::Status eState;

	shared_mutex oMsgMutex;
};


export int HotReloadClient(DNClientProxy* client, const string& ip, int port);

module:private;

DNClientProxy::DNClientProxy()
{
	iMsgId = ATOMIC_VAR_INIT(0);
	mMsgList.clear();
	bIsRegisted = false;
	pRegistEvent = nullptr;
	eState = hv::Channel::Status::CLOSED;
}

DNClientProxy::~DNClientProxy()
{
	for(auto& [k,v] : mMsgList)
	{
		v->CallResume();
	}
		
	mMsgList.clear();
}

int HotReloadClient(DNClientProxy *client, const string& ip, int port)
{
	auto reconn_setting = new reconn_setting_t;
	memcpy(reconn_setting, client->reconn_setting, sizeof reconn_setting);
	auto unpack_setting = client->unpack_setting;
	auto onConnection = client->onConnection;
	auto onMessage = client->onMessage;

	client->stop();
	delete client;
	client = new DNClientProxy;

	client->reconn_setting = reconn_setting;
	client->unpack_setting = unpack_setting;
	

	client->createsocket(port, ip.c_str());
	client->onConnection = onConnection;
	client->onMessage = onMessage;
	client->start();
	return 0;
}