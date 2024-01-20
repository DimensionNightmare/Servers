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
	DNServer():emServerType(ServerType::None),bInRun(false){};
	virtual ~DNServer(){};

public:

	virtual bool Init(map<string, string> &param) = 0;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) = 0;

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

	virtual void Pause() = 0;

	virtual void Resume() = 0;

    ServerType GetServerType(){return emServerType;}

	virtual void LoopEvent(function<void(hv::EventLoopPtr)> func){}

	bool IsRun(){ return bInRun;}
	void SetRun(bool state){bInRun = state;}

public: // dll override

protected:
    ServerType emServerType;

	bool bInRun;
};