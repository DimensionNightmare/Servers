#include "hv/TcpClient.h"
#include "hv/htime.h"
#include <iostream>

using namespace hv;

int main(){
    TcpClient cli;
	int port = 555;
    int connfd = cli.createsocket(port);
    if (connfd < 0) {
        return -20;
    }
    printf("client connect to port %d, connfd=%d ...\n", port, connfd);
    cli.onConnection = [&cli](const SocketChannelPtr& channel) {
        std::string peeraddr = channel->peeraddr();
        if (channel->isConnected()) {
            printf("connected to %s! connfd=%d\n", peeraddr.c_str(), channel->fd());
            // send(time) every 3s
            setInterval(3000, [channel](TimerID timerID){
                if (channel->isConnected()) {
                    char str[DATETIME_FMT_BUFLEN] = {0};
                    datetime_t dt = datetime_now();
                    datetime_fmt(&dt, str);
                    channel->write(str);
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
	std::string str;
    while (std::getline(std::cin, str)) {
        if (str == "stop") {
            cli.stop();
            break;
        }
    }

    return 0;
}
