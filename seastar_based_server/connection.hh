#pragma once

#include <vector>
#include <seastar/core/seastar.hh>
#include "seastar/net/api.hh"
#include <seastar/net/inet_address.hh>


namespace DistributedLogger{

	class Connection : public seastar::enable_lw_shared_from_this<Connection> {
		public:
			Connection(seastar::connected_socket fd, seastar::socket_address addr): _fd(std::move(fd)), _addr(addr), _in(_fd.input()), _out(_fd.output()) {
			}
			~Connection(){fmt::print("{} {}\n",__func__,__LINE__);}
			seastar::future<seastar::temporary_buffer<char>> receive(uint32_t len){
				try{
					if (_in.eof()){
						co_return seastar::temporary_buffer<char>();
					}
					co_return co_await _in.read_exactly(len);
				}catch(std::exception_ptr e){
					fmt::print("{} {} {}\n",__FILE__,__LINE__,e);
				}
				co_return seastar::temporary_buffer<char>();
			}
			bool isAlive() { return !_in.eof(); }
		private:
			seastar::connected_socket _fd;
			seastar::socket_address _addr;
			seastar::input_stream<char> _in;
			seastar::output_stream<char> _out;
	};
}
