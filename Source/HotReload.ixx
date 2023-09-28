#include <iostream>
#include "hv/HttpServer.h"

#ifdef HOTRELOAD_BUILD
#define MY_LIBRARY_API __declspec(dllexport)
#else
#define MY_LIBRARY_API __declspec(dllimport)
#endif

using namespace hv;


#ifdef __cplusplus
extern "C" {
#endif

MY_LIBRARY_API int ServerInit(HttpServer*  server, HttpService* router);

#ifdef __cplusplus
}
#endif

int ServerInit(HttpServer*  server, HttpService* router)
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