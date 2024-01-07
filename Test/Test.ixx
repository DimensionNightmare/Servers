#include <iostream>
#include <coroutine>
#include <map>
#include <unordered_map>
#include <fstream>
#include "schema.pb.h"
// #include <Windows.h>
#include <locale>
#include <random>
#include "hv/TcpClient.h"
#include <string>
#include "hv/EventLoop.h"
#include "hv/hloop.h" 
class A
{
public:
	A() {}
	// virtual
	~A() {}

public:
	 void msg() { std::cout << "A" << std::endl; }

private:
	int a;
};

class B : public A
{
public:
B() {}
	// virtual
	~B() {}
	 void msg() { std::cout << "B" << std::endl; }
};

int main1() 
{
	GCfg::CharacterPlayer Weapons;
	std::ifstream input("D:\\Project\\DimensionNightmare\\Environment\\GameConfig\\Gen\\Data\\character_player.bytes", std::ios::in | std::ios::binary);
	if(input)
	{
		if(Weapons.ParseFromIstream(&input))
		{
			Weapons.Clear();
			auto map = Weapons.data_map();
			for(auto one:map)
			{
				std::cout << "key" << one.first << std::endl;
				std::cout << "value"<< one.second.name() << std::endl;
			}
			auto find = map.find(55);
			std::cout << "success" << std::endl;
		}

	}
	
	std::random_device rd;
    std::mt19937 gen(rd());
	std::bernoulli_distribution  u;
	for(int i = 0; i < 5; i++)
	std::cout << u(gen) << std::endl;

	std::hash<std::string> hashstr;

	std::string msgName = GCfg::CharacterPlayer::GetDescriptor()->full_name();
	std::cout << msgName.size() << " " << msgName.length() << " " <<  strlen(msgName.c_str()) << std::endl;
	auto hashres = hashstr.operator()("");
	std::cout << size_t(hashres) << " " <<  hashres << std::endl;

	using namespace std;
	A a;
	B* b = (B*)&a;
	b->msg();
	B c;
	// c.msg();
	A* d = &c;
	d->msg();
	cout << sizeof(a) << endl;

    return 0;
}


int main()
{
	using namespace hv;
	using namespace std;
	TcpClient* pCSock = new TcpClient;
	pCSock->createsocket(1270);

	auto onConnection = [](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();


			if (channel->isConnected())
			{
				printf("%s->%s connected! connfd=%d id=%d \n", __FUNCTION__, peeraddr.c_str(), channel->fd(), channel->id());
			}
			else
			{
				printf("%s->%s disconnected! connfd=%d id=%d \n", __FUNCTION__, peeraddr.c_str(), channel->fd(), channel->id());
			}

		};

	auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) 
	{
		
	};

	pCSock->onConnection = onConnection;
	pCSock->onMessage = onMessage;

	stringstream ss;
	string str;
    while (true) 
	{
		getline(cin, str);
		ss.str(str);
		ss << str;
		ss >> str;
		if(str.empty())
		{
			continue;
		}
		
        if (str == "quit") 
		{
            break;
        }
		else if(str == "start")
		{
			auto loop = ((EventLoopThread*)pCSock)->loop();
			if(!loop->loop())
			{
				pCSock->TcpClientTmpl::TcpClientTmpl();
			}
			pCSock->start();
		}
		else if(str == "stop")
		{
			pCSock->stop();
		}
    }

	return 0;
}