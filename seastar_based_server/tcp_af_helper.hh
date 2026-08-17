#pragma once

#include <vector>
#include "seastar/net/api.hh"
#include "seastar/net/dns.hh"
#include <seastar/net/inet_address.hh>
#include "af_helper.hh"

namespace DistributedLogger{

	class TcpAfHelper : public AfHelper{
		public:
			TcpAfHelper(const seastar::sstring&host, uint16_t port){
				_host = host;
				_port = port;
			}
			seastar::future<> listen() override{
				seastar::net::dns_resolver resolver;
    
				return resolver.resolve_name(_host).then([this] (seastar::net::inet_address addr) {
					fmt::print("Resolved IP: {}\n", addr);
					_address = seastar::make_ipv4_address({addr, _port});
				}).finally([resolver0 = std::move(resolver),this] () mutable {
					return resolver0.close().then([this]{
						seastar::listen_options lo;
						lo.reuse_address = true;
						lo.set_fixed_cpu(seastar::this_shard_id());
						_listener = seastar::listen(getAddress(), lo);
						return seastar::make_ready_future<>();
					});
				});
			}
			seastar::future<seastar::connected_socket> connect() override{
				throw (std::logic_error("getAddress is not implemented"));
			}
			seastar::future<seastar::accept_result> accept() override{
				return _listener.accept();
			}
			seastar::socket_address getAddress() override {
				return _address;
			}
		private:
			seastar::sstring _host;
			uint16_t _port;
			seastar::socket_address _address;
			seastar::server_socket _listener;
	};
}
