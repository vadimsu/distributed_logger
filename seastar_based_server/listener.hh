#pragma once

#include <vector>
#include <unordered_map>
#include <seastar/core/seastar.hh>
#include "seastar/net/api.hh"
#include <seastar/net/inet_address.hh>
#include "protocol.hh"
#include "af_helper.hh"
#include "storage.hh"

namespace DistributedLogger{

class Connection;

	class Listener : public seastar::enable_lw_shared_from_this<Listener> {
		public:
			Listener(std::shared_ptr<AfHelper> afHelper): _afHelper(afHelper){}
			~Listener(){fmt::print("{} {}\n",__func__,__LINE__);}
			seastar::future<> listen(){
				return _afHelper->listen().then([this]{
						return seastar::do_until([this]{
									return false;
								},
								[this]{
									return _afHelper->accept().then([this] (auto ar) mutable {
											seastar::connected_socket fd = std::move(ar.connection);
											seastar::socket_address addr = std::move(ar.remote_address);
											auto protocol = seastar::make_lw_shared<Protocol>(addr);
							                                auto conn = seastar::make_lw_shared<Connection>(std::move(fd), addr);
											fmt::print("accepted {}\n",addr);
											_protocols.push_back(protocol);
											protocol->setStorageParams(_storageParams);
											(void)protocol->onAccepted(conn);
										});
								});
					});
			}
			void setStorageParams(std::unordered_map<seastar::sstring, seastar::sstring>& params){
				_storageParams.swap(params);
				_storage = Storage::Init(_storageParams);
			}
		private:
			std::shared_ptr<AfHelper> _afHelper;
			std::vector<seastar::lw_shared_ptr<Protocol>> _protocols;
			std::unordered_map<seastar::sstring, seastar::sstring> _storageParams;
			std::shared_ptr<Storage> _storage;
	};
}
