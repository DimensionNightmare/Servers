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
#include "hv/requests.h"
#include <signal.h>
#include <thread>
#include <chrono>
#include "pqxx/pqxx"

using namespace hv;

using namespace std;

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

void request(HttpClient* cli, HttpRequestPtr req, int num) 
{
    int ret = cli->sendAsync(req, [=](const HttpResponsePtr& resp) {
        // printf("test_http_async_client response thread tid=%ld\n", hv_gettid());
        if (resp == NULL) {
            printf("request failed! %d\n", num);
        } else {
            // printf("%d %s\r\n", resp->status_code, resp->status_message());
            // printf("%s\n", resp->body.c_str());
			printf("request success! %d\n", num);
        }
    });
}


int main2()
{	 
    // size_t filesize = requests::downloadFile("http://127.0.0.1:1212/DimensionNightmareServer.pdb", "DimensionNightmareServer.pdb");
    // if (filesize == 0) {
    //     printf("downloadFile failed!\n");
    // } else {
    //     printf("downloadFile success!\n");
    // } 

	HttpClient cli;

	HttpRequestPtr req = std::make_shared<HttpRequest>();
    req->method = HTTP_POST;
    // req->url = "http://127.0.0.1:1212/Login/Auth";
	req->url = "http://114.55.30.239:10300/Login/Auth";
    req->headers["Connection"] = "keep-alive";
	req->headers["Content-Type"] = "application/json";
    req->timeout = 10;

	hv::Json jroot;
    jroot["username"] = "admin";
    jroot["password"] = "123456";
    
	req->body = jroot.dump();

	std::array<std::thread, 1500> threads;

	for (size_t i = 0; i < threads.size(); i++)
	{
		// threads[i] =  std::thread(request, &cli, req, ref(i));
		HttpResponse resp;
		int ret = cli.send(req.get(), &resp);
		if (ret != 0)
		{
			printf("request failed! %zd\n", i);
		} 
		else 
		{
			printf("request success! %zd\n", i);
		}
		
	}

	// for (auto & ele : threads)
	// {
	// 	ele.join();
	// }
	
	// this_thread::sleep_for(20s);
    
	return 0;
}

enum class ServerType : int
{
    None,
    ControlServer,
    GlobalServer,
	AuthServer,
	GateServer,
	DatabaseServer,
	LogicServer,
};

int main()
{
	try
	{
		printf("1");
		//"postgresql://root@localhost"
		pqxx::connection c;
		printf("2");
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	
	
	// pqxx::work w(c);
	// pqxx::row r = w.exec1("SELECT 1, 2, 'Hello'");
	// auto [one, two, hello] = r.as<int, int, std::string>();
	// printf("%s,%d",hello.c_str(), one + two);
}