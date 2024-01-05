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
};

export class DNServer
{
public:
	DNServer():emServerType(ServerType::None){};
	virtual ~DNServer(){};

public:

	virtual bool Init(map<string, string> &param) = 0;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) = 0;

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

    ServerType GetServerType(){return emServerType;}

	virtual void LoopEvent(function<void(hv::EventLoopPtr)> func){}

public: // dll override

protected:
    ServerType emServerType;
};