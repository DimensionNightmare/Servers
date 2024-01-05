module;
#include "google/protobuf/Message.h"
#include "hv/Channel.h"
#include "hv/HttpServer.h"
#include "hv/HThread.h"

export module AuthServerInit;

import DNServer;
import AuthServer;
import AuthServerHelper;
import MessagePack;

using namespace hv;
using namespace std;
using namespace google::protobuf;

export void HandleAuthServerInit(DNServer *server);
export void HandleAuthServerShutdown(DNServer *server);

module:private;

void HandleAuthServerInit(DNServer *server)
{
	SetAuthServer(static_cast<AuthServer*>(server));

	auto serverProxy = GetAuthServer();

	if(auto sSock = serverProxy->GetSSock())
	{
		if(sSock->service != nullptr)
		{
			delete sSock->service;
			sSock->service = nullptr;
		}

		HttpService* service = new HttpService;
		
		service->Static("/", "./html");

		service->EnableForwardProxy();

		service->AddTrustProxy("*httpbin.org");

		service->Proxy("/httpbin/", "http://httpbin.org/");

		service->GET("/ping", [](HttpRequest* req, HttpResponse* resp) {
			return resp->String("pong");
		});

		service->GET("/data", [](HttpRequest* req, HttpResponse* resp) {
			static char data[] = "0123456789";
			return resp->Data(data, 10 /*, false */);
		});

		service->GET("/paths", [service](HttpRequest* req, HttpResponse* resp) {
			return resp->Json(service->Paths());
		});

		service->GET("/get", [](const HttpContextPtr& ctx) {
			hv::Json resp;
			resp["origin"] = ctx->ip();
			resp["url"] = ctx->url();
			resp["args"] = ctx->params();
			resp["headers"] = ctx->headers();
			return ctx->send(resp.dump(2));
		});

		service->POST("/echo", [](const HttpContextPtr& ctx) {
			return ctx->send(ctx->body(), ctx->type());
		});

		service->GET("/user/{id}", [](const HttpContextPtr& ctx) {
			hv::Json resp;
			resp["id"] = ctx->param("id");
			return ctx->send(resp.dump(2));
		});

		service->GET("/async", [](const HttpRequestPtr& req, const HttpResponseWriterPtr& writer) {
			writer->Begin();
			writer->WriteHeader("X-Response-tid", hv_gettid());
			writer->WriteHeader("Content-Type", "text/plain");
			writer->WriteBody("This is an async response.\n");
			writer->WriteBody("This is an async response.\n");
			writer->WriteBody("This is an async response.\n");
			writer->WriteBody("This is an async response.\n");
			writer->WriteBody("This is an async response.\n");
			writer->WriteBody("This is an async response.\n");
			writer->End();
		});

		service->AllowCORS();

		service->Use([](HttpRequest* req, HttpResponse* resp) {
			resp->SetHeader("X-Request-tid", hv::to_string(hv_gettid()));
			return HTTP_STATUS_NEXT;
		});

		sSock->registerHttpService(service);
	}

	if (auto cSock = serverProxy->GetCSock())
	{
		auto onConnection = [](const SocketChannelPtr &channel)
		{
			string peeraddr = channel->peeraddr();
			auto serverProxy = GetAuthServer();

			if (channel->isConnected())
			{
				printf("%s->%s connected! connfd=%d id=%d \n", __FUNCTION__, peeraddr.c_str(), channel->fd(), channel->id());

				// send RegistInfo
				serverProxy->GetCSock()->StartRegist();
			}
			else
			{
				printf("%s->%s disconnected! connfd=%d id=%d \n", __FUNCTION__, peeraddr.c_str(), channel->fd(), channel->id());
				serverProxy->GetCSock()->SetRegisted(false);
			}

			if(serverProxy->GetCSock()->isReconnect())
			{
				
			}
		};

		auto onMessage = [](const SocketChannelPtr &channel, Buffer *buf) 
		{
			MessagePacket packet;
			memcpy(&packet, buf->data(), MessagePacket::PackLenth);
			if(packet.dealType == MsgDeal::Res)
			{
				auto& reqMap = GetAuthServer()->GetCSock()->GetMsgMap();
				if(reqMap.contains(packet.msgId)) //client sock request
				{
					auto task = reqMap.at(packet.msgId);
					reqMap.erase(packet.msgId);
					task->Resume();
					Message* message = ((DNTask<Message*>*)task)->GetResult();
					message->ParseFromArray((const char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
					task->CallResume();
				}
				else
				{
					fprintf(stderr, "%s->cant find msgid! \n", __FUNCTION__);
				}
			}
			else if(packet.dealType == MsgDeal::Req)
			{
				MessagePacket packet;
				memcpy(&packet, buf->data(), MessagePacket::PackLenth);
				if(packet.dealType == MsgDeal::Req)
				{
					string msgData((char*)buf->data() + MessagePacket::PackLenth, packet.pkgLenth);
					//GlobalMessageHandle::MsgHandle(channel, packet.msgId, packet.msgHashId, msgData);
				}
			}
		};

		cSock->onConnection = onConnection;
		cSock->onMessage = onMessage;
		// cSock->SetRegistEvent(&Msg_RegistSrv);
	}

}

void HandleAuthServerShutdown(DNServer *server)
{
	auto serverProxy = GetAuthServer();

	if (auto sSock = serverProxy->GetSSock())
	{
		
	}

	if (auto cSock = serverProxy->GetCSock())
	{
		cSock->onConnection = nullptr;
		cSock->onMessage = nullptr;
		cSock->SetRegistEvent(nullptr);
	}
}