module;
#include "hv/hasync.h"
#include "hv/HttpServer.h"
#include "hv/TcpServer.h"
#include "hv/TcpClient.h"
export module ThirdParty.Libhv;

using namespace hv;
using namespace nlohmann;

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
	using ::hio_t;
	using ::EventLoop;
	using ::sockaddr_in;
	using ::sockaddr;
	using ::load_balance_e;
	using ::hssl_ctx_opt_t;
	using ::unpack_setting_t;
	using ::EventLoopThreadPool;
	using ::hevent_t;
	using ::reconn_setting_t;
	using ::sockaddr_u;
};

export namespace HVExport
{

	void cleanup() { ::async::cleanup(); }

	void hio_setcb_close(hio_t* io, const hclose_cb& close_cb) { ::hio_setcb_close(io, close_cb); }

	int getsockname(const SOCKET& s, sockaddr* name, int* namelen) { return ::getsockname(s, name, namelen); }

	u_short ntohs(u_short netshort) { return ::ntohs(netshort); }

	int Listen(int port, const char* host = "0.0.0.0") { return ::Listen(port, host); }

	uint32_t hio_id(hio_t* io) { return ::hio_id(io); }

	int hio_close(hio_t* io) { return ::hio_close(io); }

	EventLoop* tlsEventLoop() { return ::tlsEventLoop(); }

	void hio_attach(hloop_t* loop, hio_t* io) { return ::hio_attach(loop, io); }

	void hio_detach(hio_t* io) { ::hio_detach(io); }

	hio_t* haccept(hloop_t* loop, int listenfd, const haccept_cb& accept_cb) { return ::haccept(loop, listenfd, accept_cb); }

	int  hio_enable_ssl(hio_t* io) { return ::hio_enable_ssl(io); }

	int  hio_new_ssl_ctx(hio_t* io, hssl_ctx_opt_t* opt) { return ::hio_new_ssl_ctx(io, opt); }

	int sockaddr_set_ipport(sockaddr_u* addr, const char* host, int port) { return ::sockaddr_set_ipport(addr, host, port); }

	socklen_t sockaddr_len(sockaddr_u* addr) { return ::sockaddr_len(addr); }

	hio_t* hio_get(hloop_t* loop, int fd) { return ::hio_get(loop, fd); }

	bool is_ipaddr(const char* host) { return ::is_ipaddr(host); }

	bool reconn_setting_can_retry(reconn_setting_t* reconn) { return ::reconn_setting_can_retry(reconn); }

	uint32_t reconn_setting_calc_delay(reconn_setting_t* reconn) { return ::reconn_setting_calc_delay(reconn); }

	int hio_close_async(hio_t* io) { return ::hio_close_async(io); }

	SOCKET socket(int af, int type, int protocol) { return ::socket(af, type, protocol); }

	void hio_set_peeraddr(hio_t* io, struct sockaddr* addr, int addrlen) { ::hio_set_peeraddr(io, addr, addrlen); }

#pragma push_macro("hlog_disable")
#undef hlog_disable
	void hlog_disable()
	{
#pragma pop_macro("hlog_disable")
		hlog_disable();
	}

#pragma push_macro("HV_FREE")
#undef HV_FREE
	void HV_FREE(void* pointer)
	{
#pragma pop_macro("HV_FREE")
		if(pointer)
		{
			hv_free(pointer);
		}
	}

#pragma push_macro("HV_ALLOC_SIZEOF")
#undef HV_ALLOC_SIZEOF
	void* HV_ALLOC_SIZEOF(int size)
	{
		return hv_zalloc(size);
#pragma pop_macro("HV_ALLOC_SIZEOF")
	}

};
