module;
#include "hv/TcpServer.h"
#include "hv/TcpClient.h"
export module BaseServer;

using namespace std;
using namespace hv;

export enum class ServerType : int;
export class BaseServer;
export class DnServer;
export class DnClient;


enum class ServerType : int
{
    None,
    ControlServer,
    GlobalServer,

    SessionServer,
};

class BaseServer
{

public:
    BaseServer();

	virtual bool Init(map<string, string> &param) = 0;

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

    inline ServerType GetServerType(){return emServerType;};

	inline virtual void LoopEvent(function<void(EventLoopPtr)> func){};

public:
	unsigned int iMsgId;

protected:
    ServerType emServerType;
};

class DnServer : public TcpServer
{
public:

};

class DnClient : public TcpClient
{
public:
	// only oddnumber
	unsigned int iMsgId;
};

module:private;

BaseServer::BaseServer()
{
    emServerType = ServerType::None;
	iMsgId = 1;
}