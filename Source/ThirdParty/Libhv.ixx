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

export 
{
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
	using ::sockaddr_in;
	using ::sockaddr;
	using ::reconn_setting_t;
};

export namespace LibhvExport
{
	
	void cleanup()
	{
		::async::cleanup();
	}

#pragma push_macro("hlog_disable")
#undef hlog_disable
	void hlog_disable()
	{
#pragma pop_macro("hlog_disable")
		hlog_disable();
	}

	void hio_setcb_close(hio_t* io, hclose_cb close_cb) {  ::hio_setcb_close(io, close_cb);}
	
	int getsockname(const SOCKET& s, sockaddr *name, int *namelen){ return ::getsockname(s, name, namelen); }

	u_short ntohs(u_short netshort){ return ::ntohs(netshort); }
};
