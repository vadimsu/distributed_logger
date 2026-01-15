#pragma once

#include <list>
#include <seastar/core/seastar.hh>
#include <seastar/core/temporary_buffer.hh>
#include <seastar/core/iostream.hh>
#include <seastar/net/api.hh>
#include "IIO.hh"

namespace distributed_logger {

class SeastarIO : public IIO{
        public:
		SeastarIO(const seastar::sstring&, unsigned int);
		virtual ~SeastarIO();
                virtual std::shared_ptr<IBufferWrapper> send(std::shared_ptr<IBufferWrapper>) override;
		virtual void connect();
		virtual bool isConnected(){ return _connected; }
		virtual seastar::future<> disconnect();
	protected:
		virtual void reconnect();
		void initializeConnectedSocket(seastar::connected_socket);
		seastar::timer<> _reconnect_timer;
		seastar::socket_address _address;
	private:
		seastar::connected_socket _socket;
		seastar::input_stream<char> _in;
		seastar::output_stream<char> _out;
		std::list<seastar::temporary_buffer<char>> _wqueue;
		bool _connected;
		bool _transmitting;
};

}
