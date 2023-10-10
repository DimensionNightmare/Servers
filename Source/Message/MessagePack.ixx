module;
#include <string>
export module MessagePack;

using namespace std;

class Message;

enum class MsgDir : unsigned char
{
    Outer, //Client Msg
    Inner, //Server Msg
};

struct MessagePacket
{
    unsigned int pkgLenth;
    MsgDir opType;
    unsigned int msgId;

    Message* reqData; //virtual
    Message* retData; //virtual
    MessagePacket()
    {
        memset(this, 0 , sizeof *this);
    }
};

export bool MessagePack(Message* reqMsg, MessagePacket& packet)
{
    if(reqMsg == nullptr)
        return false;

    // mp.opType = MsgDir::Outer;
    // mp.msgId = msgId;
    
    string name;
    
    return true;
}

export bool MessageUnpack(char* reqMsg, int len)
{
    return true;
}