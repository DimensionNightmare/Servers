#include "hv/HttpServer.h"
#include "hv/hthread.h"    // import hv_gettid
#include "hv/hasync.h"     // import hv::async

using namespace hv;

int main()
{
	// HttpService router;
    //  router.GET("/ping", [](HttpRequest* req, HttpResponse* resp) {
    //      return resp->String("pong");
    //  });

    //  router.GET("/data", [](HttpRequest* req, HttpResponse* resp) {
    //      static char data[] = "0123456789";
    //      return resp->Data(data, 10 /*, false */);
    //  });

    //  router.GET("/paths", [&router](HttpRequest* req, HttpResponse* resp) {
    //      return resp->Json(router.Paths());
    //  });

    //  router.GET("/get", [](HttpRequest* req, HttpResponse* resp) {
    //      resp->json["origin"] = req->client_addr.ip;
    //      resp->json["url"] = req->url;
    //      resp->json["args"] = req->query_params;
    //      resp->json["headers"] = req->headers;
    //      return 200;
    //  });

    //  router.POST("/echo", [](const HttpContextPtr& ctx) {
    //      return ctx->send(ctx->body(), ctx->type());
    //  });

    //  http_server_t server;
    //  server.service = &router;
    //  server.port = 80;

    //  // uncomment to test multi-processes
    //  server.worker_processes = 4;
    //  // uncomment to test multi-threads
    //  server.worker_threads = 4;

    //  http_server_run(&server, 0);

    //  // press Enter to stop
    //  while (getchar() != '\n');
    //  http_server_stop(&server);
    //  return 0;

    int port = 0;
    port = 8080;

    HttpService router;

    /* Static file service */
    // curl -v http://ip:port/
    router.Static("/", "./html");

    /* Forward proxy service */
    router.EnableForwardProxy();
    // curl -v http://httpbin.org/get --proxy http://127.0.0.1:8080
    router.AddTrustProxy("*httpbin.org");

    /* Reverse proxy service */
    // curl -v http://ip:port/httpbin/get
    router.Proxy("/httpbin/", "http://httpbin.org/");

    /* API handlers */
    // curl -v http://ip:port/ping
    router.GET("/ping", [](HttpRequest* req, HttpResponse* resp) {
        return resp->String("pong");
    });

    // curl -v http://ip:port/data
    router.GET("/data", [](HttpRequest* req, HttpResponse* resp) {
        static char data[] = "0123456789";
        return resp->Data(data, 10 /*, false */);
    });

    // curl -v http://ip:port/paths
    router.GET("/paths", [&router](HttpRequest* req, HttpResponse* resp) {
        return resp->Json(router.Paths());
    });

    // curl -v http://ip:port/get?env=1
    router.GET("/get", [](const HttpContextPtr& ctx) {
        hv::Json resp;
        resp["origin"] = ctx->ip();
        resp["url"] = ctx->url();
        resp["args"] = ctx->params();
        resp["headers"] = ctx->headers();
        return ctx->send(resp.dump(2));
    });

    // curl -v http://ip:port/echo -d "hello,world!"
    router.POST("/echo", [](const HttpContextPtr& ctx) {
        return ctx->send(ctx->body(), ctx->type());
    });

    // curl -v http://ip:port/user/123
    router.GET("/user/{id}", [](const HttpContextPtr& ctx) {
        hv::Json resp;
        resp["id"] = ctx->param("id");
        return ctx->send(resp.dump(2));
    });

    // curl -v http://ip:port/async
    router.GET("/async", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer) {
        writer->Begin();
        writer->WriteHeader("X-Response-tid", hv_gettid());
        writer->WriteHeader("Content-Type", "text/plain");
        writer->WriteBody("This is an async response.\n");
        writer->End();
    });

    // middleware
    router.AllowCORS();
    router.Use([](HttpRequest* req, HttpResponse* resp) {
        resp->SetHeader("X-Request-tid", hv::to_string(hv_gettid()));
        return HTTP_STATUS_NEXT;
    });

    HttpServer server;
    server.service = &router;
    server.port = port;

    // uncomment to test multi-processes
    server.setProcessNum(4);
    // uncomment to test multi-threads
    server.setThreadNum(4);

    server.start();
    while (getchar() != '\n');
        hv::async::cleanup();
}