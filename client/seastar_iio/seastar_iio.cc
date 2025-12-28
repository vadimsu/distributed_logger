
#include <chrono>
#include <memory>
#include <seastar/core/sstring.hh>
#include <seastar/net/inet_address.hh>
#include <seastar/util/log.hh>
#include "seastar_iio.hh"
#include "seastar_buffer.hh"

using namespace DistributedLogger;

Seastar_IIO::Seastar_IIO(const seastar::sstring& ip, unsigned int port){
	seastar::net::inet_address in(seastar::sstring(ip.data(), ip.size()));
        _address = seastar::make_ipv4_address({in, port});
	_connected = false;
	_transmitting = false;
}

void Seastar_IIO::reconnect(){
	if (_reconnect_timer.armed()){
		return;
	}
	std::chrono::seconds reconnectDuration(10);
	_reconnect_timer.set_callback([this] {
		seastar::connect(_address).then([this](seastar::connected_socket fd){
			seastar::net::keepalive_params kap;
			std::chrono::seconds idleDuration(1);
			std::chrono::seconds interval(5);
			std::get<0>(kap).idle = idleDuration;
			std::get<0>(kap).interval = interval;
			std::get<0>(kap).count = 5;
			fd.set_keepalive_parameters(kap);
			fd.set_keepalive(true);
			_socket = std::move(fd);
			_in = _socket.input();
			_out = _socket.output();
			_connected = true;
			return;
		}).handle_exception([this](std::exception_ptr e){
			//fmt::print("{}\n",*e);
//			std::cout<<e->what()<<std::endl;
			reconnect();
		});
        });
	_reconnect_timer.arm(reconnectDuration);
}

void Seastar_IIO::Connect(){
       reconnect();
}

std::shared_ptr<IBufferWrapper> Seastar_IIO::Send(std::shared_ptr<IBufferWrapper> buffer){
	uint32_t length = buffer->getCapacity() - 4;
        length = htonl(length);
        memcpy(buffer->getData(), &length, sizeof(length));
	auto buf = static_pointer_cast<EventBufferSeastar>(buffer);
	_wqueue.push_back(std::move(buf->getBuffer()));
	if (_transmitting || !_connected){
		return nullptr;
	}
	_transmitting = true;
        seastar::do_until([this] {
		if (_wqueue.empty() || !_connected){
			_transmitting = false;
			return true;
		}
		return false;
	},
	[this] {
		return _out.write(std::move(_wqueue.front().share())).then([this] () mutable {
			if (!_wqueue.empty())
				_wqueue.pop_front();
			if (_wqueue.empty()){
				return _out.flush();
			}
			return seastar::make_ready_future<>();
		}).handle_exception([this](std::exception_ptr e) mutable{
			fmt::print("EXCEPTION {}\n",e);
			_transmitting = false;
			_connected = false;
			return seastar::make_ready_future<>();
		});
	});
	return nullptr;
}
