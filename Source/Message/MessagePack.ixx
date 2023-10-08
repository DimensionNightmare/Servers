module;
export module MessagePack;

import std.core;

using namespace std;

struct MessageHead
{
    unsigned int Lenth;
    string a;
    char g;
    bool c;
};
 
export bool MessagePack(char* data, int len, string& packData)
{
    packData.clear();

    if(!len && !data)
        return false;

    
    return true;
}

export bool MessageUnpack(char* data, int len)
{
    return true;
}