#pragma once

#include <list>
#include <seastar/core/seastar.hh>
#include <seastar/core/temporary_buffer.hh>
#include <seastar/core/iostream.hh>
#include <seastar/net/api.hh>
#include "IIO.hh"

namespace DistributedLogger {

class Seastar_IIO : public IIO{
        public:
		Seastar_IIO(const seastar::sstring&, unsigned int);
                std::shared_ptr<IBufferWrapper> Send(std::shared_ptr<IBufferWrapper>) override;
		void Connect();
		bool isConnected(){ return _connected; }
	private:
		void reconnect();
		seastar::socket_address _address;
		seastar::connected_socket _socket;
		seastar::input_stream<char> _in;
		seastar::output_stream<char> _out;
		std::list<seastar::temporary_buffer<char>> _wqueue;
		seastar::timer<> _reconnect_timer;
		bool _connected;
		bool _transmitting;
};

}
