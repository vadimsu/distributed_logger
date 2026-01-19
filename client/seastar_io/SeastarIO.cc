
#include <chrono>
#include <memory>
#include <seastar/core/sstring.hh>
#include <seastar/net/inet_address.hh>
#include <seastar/util/log.hh>
#include <seastar/core/when_all.hh>
#include "SeastarIO.hh"
#include "SeastarBuffer.hh"

using namespace distributed_logger;

SeastarIO::SeastarIO(const seastar::sstring& ip, unsigned int port): _tx_finished(seastar::make_ready_future<>()){
	seastar::net::inet_address in(seastar::sstring(ip.data(), ip.size()));
        _address = seastar::make_ipv4_address({in, port});
	_connected = false;
	_transmitting = false;
}

SeastarIO::~SeastarIO(){
}

void SeastarIO::initializeConnectedSocket(seastar::connected_socket fd){
	seastar::net::keepalive_params kap;
	std::chrono::seconds idleDuration(1);
	std::chrono::seconds interval(5);
	std::get<0>(kap).idle = idleDuration;
	std::get<0>(kap).interval = interval;
	std::get<0>(kap).count = 5;
	fd.set_keepalive_parameters(kap);
	fd.set_keepalive(true);
	_socket = std::move(fd);
	_in = std::move(_socket.input());
	_out = std::move(_socket.output());
	_connected = true;
}

void SeastarIO::reconnect(){
	if (_reconnect_timer.armed()){
		return;
	}
	std::chrono::seconds reconnectDuration(1);
	_reconnect_timer.set_callback([this] {
		seastar::connect(_address).then([this](seastar::connected_socket fd){
			initializeConnectedSocket(std::move(fd));
		}).handle_exception([this](std::exception_ptr e){
			reconnect();
		});
        });
	_reconnect_timer.arm(reconnectDuration);
}

void SeastarIO::connect(){
       reconnect();
}

seastar::future<> SeastarIO::disconnect(){
	if (!_connected){
		return seastar::make_ready_future<>();
	}
	return _tx_finished.then([this]{
		auto out_fut = _out.close();
		auto in_fut = _in.close();
		return seastar::when_all(std::move(out_fut), std::move(in_fut)).then([this](auto futs){
			if (std::get<0>(futs).failed()){
				fmt::print("failed while waiting for in {}\n",
						std::get<0>(futs).get_exception());
			}
			if (std::get<1>(futs).failed()){
				fmt::print("failed while waiting for out {}\n",
						std::get<1>(futs).get_exception());
			}
			return seastar::make_ready_future<>();
		}).handle_exception([this] (std::exception_ptr e) {
			fmt::print("{}\n",e);
			return seastar::make_ready_future<>();
		});
	}).handle_exception([this] (std::exception_ptr e){
		fmt::print("{}\n",e);
		return seastar::make_ready_future<>();
	});
}

std::shared_ptr<IBufferWrapper> SeastarIO::send(std::shared_ptr<IBufferWrapper> buffer){
	uint32_t length = buffer->getCapacity() - 4;
        length = htonl(length);
        memcpy(buffer->getData(), &length, sizeof(length));
	auto buf = static_pointer_cast<SeastarBuffer>(buffer);
	_wqueue.push_back(std::move(buf->getBuffer()));
	if (_transmitting || !_connected){
		return nullptr;
	}
	_transmitting = true;
        _tx_finished = std::move(seastar::do_until([this] {
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
			fmt::print("{}\n",e);
			_transmitting = false;
			_connected = false;
			return seastar::make_ready_future<>();
		});
	}));
	return nullptr;
}
