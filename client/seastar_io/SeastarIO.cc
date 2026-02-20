
#include <chrono>
#include <memory>
#include <seastar/core/sstring.hh>
#include <seastar/net/inet_address.hh>
#include <seastar/util/log.hh>
#include <seastar/core/when_all.hh>
#include "SeastarIO.hh"
#include "SeastarBuffer.hh"

using namespace distributed_logger;

SeastarIO::SeastarIO(const seastar::sstring& ip, unsigned int port) noexcept: _tx_finished(seastar::make_ready_future<>()){
	seastar::net::inet_address in(seastar::sstring(ip.data(), ip.size()));
        _address = seastar::make_ipv4_address({in, static_cast<uint16_t>(port)});
	_connected = false;
	_transmitting = false;
	_queuemaxsize = 0;
	_queuesize = 0;
	_logPostedCnt = 0;
	_logDroppedCnt = 0;
	_logSentCnt = 0;
	_shutting_down = false;
}

SeastarIO::~SeastarIO() noexcept {
}

void SeastarIO::initializeConnectedSocket(seastar::connected_socket fd) noexcept{
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
}

void SeastarIO::reconnect() noexcept {
	if (_reconnect_timer.armed()){
		return;
	}
	std::chrono::seconds reconnectDuration(1);
	_reconnect_timer.set_callback([this] {
		(void)seastar::connect(_address).then([this](seastar::connected_socket fd){
			initializeConnectedSocket(std::move(fd));
		}).handle_exception([this](std::exception_ptr e){
			reconnect();
		});
        });
	_reconnect_timer.arm(reconnectDuration);
}

void SeastarIO::connect() noexcept {
       reconnect();
}

seastar::future<> SeastarIO::disconnect() noexcept {
	if (!_connected){
		return seastar::make_ready_future<>();
	}
	_shutting_down = true;
	return _tx_finished.then([this]{
		_socket.shutdown_output();
		_socket.shutdown_input();
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

std::shared_ptr<IBufferWrapper> SeastarIO::send(std::shared_ptr<IBufferWrapper> buffer) noexcept {
	auto buf = static_pointer_cast<SeastarBuffer>(buffer);
	auto seastar_buf = buf->getBuffer();
	if (_queuemaxsize != 0 && _queuesize + /*buffer->getCapacity()*/seastar_buf.size() > _queuemaxsize){
		_logDroppedCnt++;
		return nullptr;
	}
	if (_shutting_down || !_connected){
		return nullptr;
	}
	_logPostedCnt++;
	uint32_t length = /*buffer->getCapacity()*/seastar_buf.size() - 4;
        length = htonl(length);
        memcpy(/*buffer->getData()*/seastar_buf.get_write(), &length, sizeof(length));
	_queuesize += /*buf->getBuffer()*/seastar_buf.size();
	_wqueue.push_back(std::move(/*buf->getBuffer()*/seastar_buf));
//	_queuesize += ntohl(length) + 4;
	if (_transmitting || !_connected){
		return nullptr;
	}
	_transmitting = true;
        _tx_finished = std::move(seastar::do_until([this] {
		if (_wqueue.empty() || !_connected || _shutting_down){
			_transmitting = false;
			return true;
		}
		return false;
	},
	[this] {
		auto bufsize = _wqueue.front().size();
		auto buf = std::move(_wqueue.front());
		_wqueue.pop_front();
		return _out.write(std::move(buf)).then([this, bufsize] () mutable {
			_queuesize -= (bufsize >= _queuesize ? _queuesize : bufsize);
			_logSentCnt++;
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
