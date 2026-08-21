#pragma once

#include <vector>
#include <unordered_map>
#include <ranges>
#include <seastar/core/seastar.hh>
#include "seastar/net/api.hh"
#include <seastar/net/inet_address.hh>
#include <seastar/core/smp.hh>
#include <boost/iterator/counting_iterator.hpp>
#include "protocol.hh"
#include "af_helper.hh"
#include "storage.hh"
#include <seastar/core/shard_id.hh>
namespace DistributedLogger{

class Connection;

	class Listener : public seastar::enable_lw_shared_from_this<Listener> {
		public:
			Listener(const seastar::sstring& ip, uint16_t port): _ip(ip), _port(port){
				_protocols.resize(seastar::smp::count);
			}
			~Listener(){}
			seastar::future<> listen(){
				auto afHelper = std::make_shared<DistributedLogger::TcpAfHelper>(_ip, _port);
				afHelper->listen().then([this, afHelper]{
					return seastar::do_until([this]{
							return false;
						},
						[this, afHelper]{
							return afHelper->accept().then([this] (auto ar) mutable {
									seastar::connected_socket fd = std::move(ar.connection);
									seastar::socket_address addr = std::move(ar.remote_address);
									auto protocol = seastar::make_lw_shared<Protocol>(addr);
					                                auto conn = seastar::make_lw_shared<Connection>(std::move(fd), addr);
									_protocols.push_back(protocol);
									protocol->setStorageParams(_storageParams);
									(void)protocol->onAccepted(conn);
									return seastar::make_ready_future<>();
								});
						});
					});
					return seastar::make_ready_future<>();
			}
			void setStorageParams(const std::unordered_map<seastar::sstring, seastar::sstring>& params){
				_storageParams = params;
			}
		private:
			seastar::sstring _ip;
			uint16_t _port;
			std::vector<seastar::lw_shared_ptr<Protocol>> _protocols;
			std::unordered_map<seastar::sstring, seastar::sstring> _storageParams;
	};
}
