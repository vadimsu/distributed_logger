#pragma once

#include <vector>
#include "seastar/net/api.hh"
#include <seastar/net/inet_address.hh>
#include "af_helper.hh"

namespace DistributedLogger{

	class TcpAfHelper : public AfHelper{
		public:
			TcpAfHelper(const seastar::sstring&ip, uint16_t port){
				fmt::print("setting up TCP listener {} {}\n",ip,port);
				seastar::net::inet_address in(seastar::sstring(ip.data(), ip.size()));
				_address = seastar::make_ipv4_address({in, port});
			}
			seastar::future<> listen() override{
				seastar::listen_options lo;
				lo.reuse_address = true;
				lo.set_fixed_cpu(seastar::this_shard_id());
				_listener = seastar::listen(getAddress(), lo);
				return seastar::make_ready_future<>();
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
			seastar::socket_address _address;
			seastar::server_socket _listener;
	};
}
