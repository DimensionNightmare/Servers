module;
#include <concepts>

#include "hv/hasync.h"
#include "hv/HttpServer.h"
#include "hv/TcpServer.h"
#include "hv/TcpClient.h"
#include "hv/json.hpp"
export module ThirdParty.Libhv;

template <typename F>
concept NoArgCallable = requires(F f) {
    { std::invoke(f) } -> std::same_as<void>;
};

template <NoArgCallable F>
auto make_wrapper(F&& f) {
    return [f=std::forward<F>(f)]() { 
        f(); 
    };
}

template <typename F>
auto make_wrapper(F&& f) requires (!NoArgCallable<F>) {
    return [f=std::forward<F>(f)](auto&&... args) -> decltype(auto) {
        return std::invoke(f, std::forward<decltype(args)>(args)...);
    };
}

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

	using ::getsockname;
	using ::ntohs;
	using ::logger_set_file;
};

export namespace hv
{
	auto cleanup = make_wrapper(async::cleanup);

	void hvlog_disable()
	{
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

export namespace nlohmann
{
	namespace detail
	{
		using nlohmann::detail::json_sax_dom_callback_parser;
	}
	using  ::nlohmann::json;
}
