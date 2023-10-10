module;
#include "hv/TcpServer.h"
export module BaseServer;

using namespace hv;
using namespace std;

export class BaseServer :public TcpServer
{
public:
    BaseServer();
    void LoopEvent(function<void(EventLoopPtr)> func);

    void SetChannelUserData(SocketChannelPtr& channel);
public:
    //Control By Self
    map<string, SocketChannelPtr> mChild;
};

BaseServer::BaseServer()
{
    mChild.clear();
}

void BaseServer::LoopEvent(function<void(EventLoopPtr)> func)
{
    map<long,EventLoopPtr> looped;
    while(EventLoopPtr pLoop = loop()){
        long id = pLoop->tid();
        if(looped.find(id) == looped.end())
        {
            func(pLoop);
            looped[id] = pLoop;
        }
        else
        {
            break;
        }
    };
    
}