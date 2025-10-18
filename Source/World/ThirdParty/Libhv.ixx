module;
#include <concepts>

#include "hv/hasync.h"
#include "hv/HttpServer.h"
#include "hv/TcpServer.h"
#include "hv/TcpClient.h"
#include "hv/json.hpp"
export module ThirdParty.Libhv;

import ECSW;

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

export namespace hv
{
	using ::HttpRequestPtr;
	using ::HttpResponseWriterPtr;
	using ::HttpContextPtr;
	using ::http_status;
	
	using ::unpack_setting_t;
	using ::reconn_setting_t;

	using hv::HttpService;
	using hv::Buffer;
	using hv::EventLoopThread;
	using hv::HttpServer;
	using hv::EventLoopThreadPool;

	using hv::TcpClientTmpl;
	using hv::TcpServerTmpl;
}

export class SocketChannel : public hv::SocketChannel
{
public:
	using Ptr = std::shared_ptr<SocketChannel>;
	using CVPtr = const Ptr&;

	virtual ~SocketChannel()
	{
		
	}

	SocketChannel(hio_t* io) : hv::SocketChannel(io)
	{
		
	}


	void SetWorld(World::WPtr world) 
	{
		pWorld = world;
	}

	World::Ptr GetWorld() { return pWorld.lock(); }
protected:

	World::WPtr pWorld;
};

export namespace Libhv
{
	auto cleanup = make_wrapper(hv::async::cleanup);

	void hvlog_disable() { hlog_disable(); }

	void Run(hv::TcpClientTmpl<SocketChannel>* obj) { obj->start(); }

	void Run(hv::TcpServerTmpl<SocketChannel>* obj) { obj->start(); }

}

export namespace nlohmann
{
	namespace detail
	{
		using nlohmann::detail::json_sax_dom_callback_parser;
	}
	using  ::nlohmann::json;
}
