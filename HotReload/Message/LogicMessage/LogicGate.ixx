module;
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Server/S_Gate.pb.h"
export module LogicMessage:LogicGate;

import LogicServerHelper;
import Logger;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

namespace LogicMessage
{
    export void Exe_RetProxyOffline(SocketChannelPtr channel, Message* msg)
    {
        g2L_RetProxyOffline* request = reinterpret_cast<g2L_RetProxyOffline*>(msg);

        LogicServerHelper* dnServer = GetLogicServer();
        ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();

        if(ClientEntity* entity = entityMan->GetEntity(request->entity_id()))
        {
            DNPrint(0, LoggerLevel::Debug, "Recv Client %u Disconnect !!", entity->ID());

            entityMan->SaveEntity(*entity);
            entityMan->RemoveEntity(entity->ID());
            return;
        }

        DNPrint(0, LoggerLevel::Debug, "Recv Client %u Disconnect but not Exist!!", request->entity_id());
    }
}