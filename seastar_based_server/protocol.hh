#pragma once

#include <unordered_map>
#include <seastar/core/seastar.hh>
#include "seastar/net/api.hh"
#include <seastar/net/inet_address.hh>
#include "connection.hh"
#include "event_decoder.hh"
#include "storage.hh"

namespace DistributedLogger{

	class Protocol : public seastar::enable_lw_shared_from_this<Protocol> {
		public:
			Protocol(seastar::socket_address addr): _addr(addr), _batchSize(0){
			}
			~Protocol(){}
			seastar::future<> onAccepted(seastar::lw_shared_ptr<Connection> connection){
				_connection = connection;
				_storage = Storage::Init(_storageParams);
				_connection->setRxBufferSize(4096*4096);
				auto it = _storageParams.find("WorkersBufferSize");
				if (it != _storageParams.end()){
					_batchSize = atoi(it->second.c_str());
					_batch.reserve(_batchSize);
				}
				try {
					while(_connection->isAlive()){
						auto  length_prefix_tb = co_await _connection->receive(4);
						if (length_prefix_tb.size() != 4){
							break;
						}
						uint32_t data_len = 0;
						memcpy(&data_len, length_prefix_tb.get(), sizeof(data_len));
						data_len = ntohl(data_len);
						auto data_buffer_tb = co_await _connection->receive(data_len);
						if (data_buffer_tb.size() != data_len){
							break;
						}
						co_await process_accumulated_bytes(std::move(data_buffer_tb));
					}
				}catch(std::exception_ptr e){
					fmt::print("{} {} {}\n",__FILE__,__LINE__,e);
				}
				co_return;
			}
			void setStorageParams(const std::unordered_map<seastar::sstring, seastar::sstring>& params){
				_storageParams = params;
			}
		private:
			seastar::future<> process_accumulated_bytes(seastar::temporary_buffer<char> tb) {
				_batch.push_back(std::move(tb));
				if (_batchSize == 0 || _batch.size() == _batchSize){
					_batchTimer.cancel();
					return _storage->Flush(std::move(_batch));
				}
				if (!_batchTimer.armed()){
					_batchTimer.set_callback([this] {
							if (_batch.size() > 0){
								_storage->Flush(std::move(_batch));
							}
						});
				}
				return seastar::make_ready_future<>();
			}
			seastar::socket_address _addr;
			seastar::lw_shared_ptr<Connection> _connection;
			std::unordered_map<seastar::sstring, seastar::sstring> _storageParams;
			std::vector<seastar::temporary_buffer<char>> _batch;
			std::shared_ptr<Storage> _storage;
			unsigned _batchSize;
			seastar::timer<> _batchTimer;
	};
}
