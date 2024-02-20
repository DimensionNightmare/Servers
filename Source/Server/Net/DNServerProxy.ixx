module;
#include "hv/TcpServer.h"
#include "google/protobuf/message.h"

#include <functional> 
#include <shared_mutex>
export module DNServerProxy;

import DNTask;

using namespace std;
using namespace google::protobuf;

export class DNServerProxy : public hv::TcpServer
{
public:
	DNServerProxy();
	~DNServerProxy(){};

public: // dll override

protected:
	// only oddnumber
	atomic<unsigned int> iMsgId;
	// unordered_
	map<unsigned int, DNTask<Message*>* > mMsgList;

	std::shared_mutex oMsgMutex;
};

module:private;

DNServerProxy::DNServerProxy()
{
	iMsgId = ATOMIC_VAR_INIT(0);
	mMsgList.clear();
}