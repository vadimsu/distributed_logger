#pragma once

#include <vector>
#include <seastar/core/seastar.hh>
#include "seastar/net/api.hh"
#include <seastar/net/inet_address.hh>
#include "connection.hh"
#include "event_decoder.hh"

namespace DistributedLogger{

	class Protocol : public seastar::enable_lw_shared_from_this<Protocol> {
		public:
			Protocol(seastar::socket_address addr): _addr(addr){}
			~Protocol(){fmt::print("{} {}\n",__func__,__LINE__);}
			seastar::future<> onAccepted(seastar::lw_shared_ptr<Connection> connection){
				_connection = connection;
				while(_connection->isAlive()){
					auto  length_prefix_tb = co_await _connection->receive(4);
					if (length_prefix_tb.size() != 4){
						break;
					}
					uint32_t data_len = 0;
					memcpy(&data_len, length_prefix_tb.get(), sizeof(data_len));
					data_len = ntohl(data_len);
					fmt::print("received length prefix {}\n",data_len);
					auto data_buffer_tb = co_await _connection->receive(data_len);
					fmt::print("received data len{}\n",data_buffer_tb.size());
					if (data_buffer_tb.size() != data_len){
						break;
					}
					process_accumulated_bytes(std::move(data_buffer_tb));
				}
				co_return;
			}
		private:
			void process_accumulated_bytes(seastar::temporary_buffer<char> tb) {
				auto event_and_rc = DecodeUint64(tb);
				if (std::get<1>(event_and_rc) == -1){
					fmt::print("failed to decoded event {}\n",std::get<1>(event_and_rc));
					return;
				}
				switch (std::get<0>(event_and_rc)){
					case 0:
						fmt::print("Event0\n");
						{
							auto event0_and_rc = Decode_event0(tb, 8);
						}
						break;
					case 1:
						fmt::print("Event1\n");
						{
							auto event1_and_rc = Decode_event1(tb, 8);
						}
						break;
					default:
						fmt::print("unknown event {}\n",std::get<0>(event_and_rc));
				}
			}
			seastar::socket_address _addr;
			seastar::lw_shared_ptr<Connection> _connection;
	};
}
