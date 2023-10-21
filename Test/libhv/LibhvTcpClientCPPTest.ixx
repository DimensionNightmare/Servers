#include "hv/TcpClient.h"
#include "hv/htime.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#include "schema.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
using namespace google::protobuf::io;

#include "google/protobuf/util/json_util.h"
using namespace google::protobuf::util;

#include "google/protobuf/message.h"

using namespace hv;
using namespace std;

import MessagePack;

int main(int argc, char** argv)
{
	map<string,string> lunchParam;
	{
		string split;
		vector<string> tokens;
		for (int i = 1; i < argc; i++)
		{
			tokens.clear();
			stringstream param(argv[i]);
			
			while (getline(param, split, '=')) 
			{
				tokens.push_back(split);
			}

			if(tokens.empty() || tokens.size() != 2)
			{
				cerr << "program lunch param error! Pos: " << i << endl;
				return 0;
			}

			lunchParam.emplace(tokens.front(), tokens.back());
		}
	}

    TcpClient cli;
	int port = 0;
	if(lunchParam.contains("port"))
	{
		port = stoi(lunchParam["port"]);
	}

    int connfd = cli.createsocket(port);
    if (connfd < 0) {
        return -20;
    }
    printf("client connect to port %d, connfd=%d ...\n", port, connfd);
    ifstream file("D://Project//DimensionNightmare//Environment//GameConfig//Gen//Data//item_weapon.bytes", ios::binary);
    IstreamInputStream input_stream(&file);
    CodedInputStream input(&input_stream);

    GCfg::ItemWeapon tblItem;
    if(!tblItem.MergeFromCodedStream(&input))
    {
        return -1;
    }

    auto table = tblItem.mutable_data_list();
    string msgStr;
    google::protobuf::Any toMsg;
    toMsg.PackFrom(table->Get(0));
    toMsg.SerializeToString(&msgStr);

    stringstream stream; 
    int dataSize = toMsg.ByteSize();
    unsigned char* pSize = (unsigned char*)&dataSize;
    string lenStr((char*)pSize, 4);
    stream << lenStr << msgStr;

    string datatest;
    //MessagePack(msgStr.data(), msgStr.length(), datatest);

    cli.onConnection = [&cli, &stream](const SocketChannelPtr& channel) {
        string peeraddr = channel->peeraddr();
        if (channel->isConnected()) {
            printf("connected to %s! connfd=%d\n", peeraddr.c_str(), channel->fd());
            // send(time) every 3s
            setInterval(1000, [channel, &stream](TimerID timerID){
                if (channel->isConnected()) {
                    // char str[DATETIME_FMT_BUFLEN] = {0};
                    // datetime_t dt = datetime_now();
                    // datetime_fmt(&dt, str);
                    
                    
                    channel->write(stream.str());
                } else {
                    killTimer(timerID);
                }
            });
        } else {
            printf("disconnected to %s! connfd=%d\n", peeraddr.c_str(), channel->fd());
        }
        if (cli.isReconnect()) {
            printf("reconnect cnt=%d, delay=%d\n", cli.reconn_setting->cur_retry_cnt, cli.reconn_setting->cur_delay);
        }
    };
    cli.onMessage = [](const SocketChannelPtr& channel, Buffer* buf) {
        printf("< %.*s\n", (int)buf->size(), (char*)buf->data());
    };
    // reconnect: 1,2,4,8,10,10,10...
    reconn_setting_t reconn;
    reconn.min_delay = 1000;
    reconn.max_delay = 10000;
    reconn.delay_policy = 2;
    cli.setReconnect(&reconn);
    cli.start();

    // press Enter to stop
	string cmd;
    while (getline(cin, cmd)) {
        if (cmd == "stop") {
            cli.stop();
            break;
        }
    }

    return 0;
}
