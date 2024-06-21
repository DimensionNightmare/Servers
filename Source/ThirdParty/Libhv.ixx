module;
#include "hv/HttpServer.h"
#include "hv/Channel.h"
#include "hv/hasync.h"
#include "hv/hlog.h"
#include "hv/EventLoop.h"
#include "hv/EventLoopThread.h"
#include "hv/hsocket.h"
#include "hv/TcpServer.h"
#include "hv/TcpClient.h"
#include "hv/hloop.h" 
#include "hv/requests.h"
#include "hv/json.hpp"
#include "hv/hloop.h"
export module ThirdParty.Libhv;

using namespace hv;

export {
	using ::SocketChannelPtr;
	using ::HttpRequestPtr;
	using ::HttpResponsePtr;
	using ::HttpResponseWriterPtr;
	using ::HttpContextPtr;
	using ::HttpService;
	using ::http_ctx_handler;
	using ::http_status;
	using ::Buffer;
	using ::EventLoopThread;
	using ::EventLoopPtr;
	using ::SocketChannel;
	using ::HttpServer;
	using ::TcpClientEventLoopTmpl;
	using ::TcpServerEventLoopTmpl;
	using ::hio_t;
	using ::TcpClient;
	using ::TcpServer;
	using ::EventLoop;
};

export {
	void HV_cleanup() { ::async::cleanup(); }
	void HV_hlog_disable() { hlog_disable(); }
};
