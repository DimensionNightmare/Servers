#include "hv/HttpServer.h"
#include "hv/hthread.h"    // import hv_gettid
#include "hv/hasync.h"     // import hv::async

int main()
{
	HttpService router;
     router.GET("/ping", [](HttpRequest* req, HttpResponse* resp) {
         return resp->String("pong");
     });

     router.GET("/data", [](HttpRequest* req, HttpResponse* resp) {
         static char data[] = "0123456789";
         return resp->Data(data, 10 /*, false */);
     });

     router.GET("/paths", [&router](HttpRequest* req, HttpResponse* resp) {
         return resp->Json(router.Paths());
     });

     router.GET("/get", [](HttpRequest* req, HttpResponse* resp) {
         resp->json["origin"] = req->client_addr.ip;
         resp->json["url"] = req->url;
         resp->json["args"] = req->query_params;
         resp->json["headers"] = req->headers;
         return 200;
     });

     router.POST("/echo", [](const HttpContextPtr& ctx) {
         return ctx->send(ctx->body(), ctx->type());
     });

     http_server_t server;
     server.service = &router;
     server.port = 80;

     // uncomment to test multi-processes
     server.worker_processes = 4;
     // uncomment to test multi-threads
     server.worker_threads = 4;

     http_server_run(&server, 0);

     // press Enter to stop
     while (getchar() != '\n');
     http_server_stop(&server);
     return 0;
}