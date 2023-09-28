

#include "hv/hv.h"
#include "hv/HttpServer.h"
#include "hv/hasync.h"
#include "hv/hthread.h"

#include <Windows.h>
#include <future>
#include <iostream>
#include <string>

using namespace hv;

int ServerInit1(HttpServer*  server, HttpService* router);


int main(int argc, char** argv)
{
	// auto handle = LoadLibrary("HotReload.dll");
	HttpServer*  server = nullptr; 
	HttpService* router = nullptr;
MAIN:
	// if(handle)
	// {
	// 	if(auto funtPtr = GetProcAddress(handle, "ServerInit"))
	// 	{
	// 		typedef int (*ServerInit)(HttpServer*,HttpService*);
	// 		auto func = reinterpret_cast<ServerInit>(funtPtr);
	// 		if(func)
	// 		{
	// 			server = new HttpServer();
	// 			router = new HttpService();
	// 			func(server, router);
	// 		}
				
	// 	}
	// }

	std::string str;
  
    while (std::getline(std::cin, str)) {
        if (str == "quit") {
            break;
        } else if (str == "start") {
				// handle = LoadLibrary("HotReload.dll");
		   		goto MAIN;
        } else if (str == "stop") {
			// if(handle)
			{
				if(server)
				{
					hv::async::cleanup();
					server = nullptr; 
				}
           		// FreeLibrary(handle);
			}
		//    handle = nullptr;
        } 
    }
	
	
	return 0;
}

int ServerInit1(HttpServer*  server, HttpService* router)
{
    /* Static file service */
    // curl -v http://ip:port/
    router->Static("/", "./html");

    /* Forward proxy service */
    router->EnableForwardProxy();
    // curl -v http://httpbin.org/get --proxy http://127.0.0.1:8080
    router->AddTrustProxy("*httpbin.org");

    /* Reverse proxy service */
    // curl -v http://ip:port/httpbin/get
    router->Proxy("/httpbin/", "http://httpbin.org/");

    /* API handlers */
    // curl -v http://ip:port/ping
    router->GET("/ping", [](HttpRequest* req, HttpResponse* resp) {
        return resp->String("pong");
    });

    // curl -v http://ip:port/data
    router->GET("/data", [](HttpRequest* req, HttpResponse* resp) {
        static char data[] = "0123456789";
        return resp->Data(data, 10 /*, false */);
    });

    // curl -v http://ip:port/paths
    router->GET("/paths", [&router](HttpRequest* req, HttpResponse* resp) {
        return resp->Json(router->Paths());
    });

    // curl -v http://ip:port/get?env=1
    router->GET("/get", [](const HttpContextPtr& ctx) {
        hv::Json resp;
        resp["origin"] = ctx->ip();
        resp["url"] = ctx->url();
        resp["args"] = ctx->params();
        resp["headers"] = ctx->headers();
        return ctx->send(resp.dump(2));
    });

    // curl -v http://ip:port/echo -d "hello,world!"
    router->POST("/echo", [](const HttpContextPtr& ctx) {
        return ctx->send(ctx->body(), ctx->type());
    });

    // curl -v http://ip:port/user/123
    router->GET("/user/{id}", [](const HttpContextPtr& ctx) {
        hv::Json resp;
        resp["id"] = ctx->param("id");
        return ctx->send(resp.dump(2));
    });

	if(server == nullptr)
    	server = new HttpServer();

    server->service = router;
    server->port = 8080;

    // uncomment to test multi-processes
    server->setProcessNum(4);
    // uncomment to test multi-threads
    server->setThreadNum(4);

    server->start();
    return 0;
}