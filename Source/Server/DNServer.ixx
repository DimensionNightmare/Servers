module;
#include "hv/EventLoop.h"

#include <functional> 
export module DNServer;

using namespace std;

export enum class ServerType : unsigned char
{
    None,
    ControlServer,
    GlobalServer,
	AuthServer,

	GateServer,
	DatabaseServer,
	LogicServer,
	
	Max,
};

export class DNServer
{
public:
	DNServer();
	virtual ~DNServer(){};

public:

	virtual bool Init(map<string, string> &param);

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) = 0;

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

	virtual void Pause() = 0;

	virtual void Resume() = 0;

    ServerType GetServerType(){return emServerType;}
	unsigned int GetServerIndex(){return iServerIndex;}
	void SetServerIndex(unsigned int serverIndex){iServerIndex = serverIndex;}

	virtual void LoopEvent(function<void(hv::EventLoopPtr)> func){}

	bool IsRun(){ return bInRun;}
	void SetRun(bool state){bInRun = state;}

	virtual void ReClientEvent(const char* ip, int port){};

public: // dll override

protected:
    ServerType emServerType;

	bool bInRun;
	unsigned int iServerIndex;
};

DNServer::DNServer()
{
	emServerType = ServerType::None;
	bInRun = false;
	iServerIndex = 0;
}

bool DNServer::Init(map<string, string> &param)
{
	if(param.contains("svrIndex"))
	{
		iServerIndex = stoi(param["svrIndex"]);
	}

	return true;
}
