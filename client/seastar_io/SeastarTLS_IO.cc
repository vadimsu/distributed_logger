
#include <chrono>
#include <memory>
#include <seastar/core/sstring.hh>
#include <seastar/net/inet_address.hh>
#include <seastar/util/log.hh>
#include "SeastarTLS_IO.hh"
#include "SeastarBuffer.hh"

using namespace distributed_logger;

SeastarTLS_IO::SeastarTLS_IO(const seastar::sstring& ip, unsigned int port,
		const seastar::sstring& certificate, const seastar::sstring& key, const seastar::sstring& trusted) noexcept: SeastarIO(ip, port){
	_certificate = certificate;
	_key = key;
	_trusted = trusted;
}

void SeastarTLS_IO::reconnect() noexcept {
	if (_reconnect_timer.armed()){
		return;
	}
	std::chrono::seconds reconnectDuration(1);
	_reconnect_timer.set_callback([this] mutable {
		(void)buildClientCredentials().then([this](auto certs){
			return seastar::tls::connect(certs, _address).then([this](seastar::connected_socket fd){
				initializeConnectedSocket(std::move(fd));
			}).handle_exception([this](std::exception_ptr e){
				reconnect();
			});
		});
	});
	_reconnect_timer.arm(reconnectDuration);
}

seastar::future<seastar::shared_ptr<seastar::tls::certificate_credentials>> SeastarTLS_IO::buildClientCredentials() noexcept{
	if (_trusted != "") {
		return _builder.set_x509_trust_file(_trusted, seastar::tls::x509_crt_format::PEM).then([this] () mutable{
			return _builder.set_x509_key_file(_certificate, _key, seastar::tls::x509_crt_format::PEM).then([this] () mutable{
				return _builder.build_certificate_credentials();
			});
		});
	}
	return _builder.set_x509_key_file(_certificate, _key, seastar::tls::x509_crt_format::PEM).then([this] () mutable{
		return _builder.build_certificate_credentials();
	});
}
