module;
#include "hv/hasync.h"
#include "hv/HttpServer.h"
#include "hv/TcpServer.h"
#include "hv/TcpClient.h"
export module ThirdParty.Libhv;

export
{
	using namespace hv;

	using ::HttpRequestPtr;
	using ::HttpResponsePtr;
	using ::HttpResponseWriterPtr;
	using ::HttpContextPtr;
	using ::http_ctx_handler;
	using ::http_status;
	using ::hio_t;
	using ::sockaddr_in;
	using ::sockaddr;
	using ::load_balance_e;
	using ::hssl_ctx_opt_t;
	using ::unpack_setting_t;
	using ::hevent_t;
	using ::reconn_setting_t;
	using ::sockaddr_u;
	using ::SocketChannelPtr;
	using ::HttpService;
	using ::Buffer;
	using ::EventLoopThread;
	using ::EventLoopPtr;
	using ::SocketChannel;
	using ::HttpServer;
	using ::EventLoop;
	using ::EventLoopThreadPool;
	using ::TcpClient;
	using ::TcpServer;
};

export namespace hv
{
	void cleanup() { async::cleanup(); }

	int getsockname(const SOCKET& s, sockaddr* name, int* namelen) { return ::getsockname(s, name, namelen); }

	u_short ntohs(u_short netshort) { return ::ntohs(netshort); }

#pragma push_macro("hlog_disable")
#undef hlog_disable
	void hlog_disable()
	{
#pragma pop_macro("hlog_disable")
		hlog_disable();
	}

	void HVRun(TcpClient* obj)
	{
		obj->start();
	}

	void HVRun(TcpServer* obj)
	{
		obj->start();
	}
}