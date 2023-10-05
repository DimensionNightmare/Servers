module;

#include "hv/hv.h"
#include "hv/TcpServer.h"

export module BaseServer;

using namespace hv;

export class BaseServer :public TcpServer
{
public:
    void LoopEvent(std::function<void(EventLoopPtr)> func);
};

void BaseServer::LoopEvent(std::function<void(EventLoopPtr)> func)
{
    std::map<long,EventLoopPtr> looped;
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
