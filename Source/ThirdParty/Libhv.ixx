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
	using ::sockaddr_in;
	using ::sockaddr;
	using ::getsockname;
	using ::ntohs;
	using ::sockaddr_u;
}

export namespace hv
{
	using ::HttpRequestPtr;
	using ::HttpResponsePtr;
	using ::HttpResponseWriterPtr;
	using ::HttpContextPtr;
	using ::http_ctx_handler;
	using ::http_status;
	
	using ::hio_t;
	using ::hssl_ctx_opt_t;
	using ::unpack_setting_t;
	using ::hevent_t;
	using ::reconn_setting_t;
	using ::logger_set_file;

	using hv::HttpService;
	using hv::Buffer;
	using hv::EventLoopThread;
	using hv::SocketChannel;
	using hv::HttpServer;
	using hv::EventLoop;
	using hv::EventLoopThreadPool;
	using hv::TcpClient;
	using hv::TcpServer;

	using SocketChannelPtr = std::shared_ptr<hv::SocketChannel>;
	using EventLoopPtr = std::shared_ptr<hv::EventLoop>;
}

export namespace Libhv
{
	auto cleanup = make_wrapper(hv::async::cleanup);

	void hvlog_disable() { hlog_disable(); }

	void Run(hv::TcpClient* obj) { obj->start(); }

	void Run(hv::TcpServer* obj) { obj->start(); }

}

export namespace nlohmann
{
	namespace detail
	{
		using nlohmann::detail::json_sax_dom_callback_parser;
	}
	using  ::nlohmann::json;
}
