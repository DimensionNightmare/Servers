module;

#include "hv/hv.h"
#include "hv/TcpServer.h"

export module DNTcpServer;

export class DNTcpServer
{
public:
    DNTcpServer(/* args */);
    ~DNTcpServer();
    
private:
    hv::TcpServer* pServer;
};

DNTcpServer::DNTcpServer()
{
    pServer = nullptr;
}

DNTcpServer::~DNTcpServer()
{
}