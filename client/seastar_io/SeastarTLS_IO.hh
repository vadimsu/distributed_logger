#pragma once

#include <list>
#include <seastar/core/seastar.hh>
#include <seastar/core/temporary_buffer.hh>
#include <seastar/core/iostream.hh>
#include <seastar/net/api.hh>
#include <seastar/net/tls.hh>
#include "SeastarIO.hh"

namespace distributed_logger {

class SeastarTLS_IO : public SeastarIO{
        public:
		SeastarTLS_IO(const seastar::sstring&, unsigned int, const seastar::sstring&, const seastar::sstring&, const seastar::sstring&) noexcept;
	protected:
		void reconnect() noexcept override;
	private:
		seastar::future<seastar::shared_ptr<seastar::tls::certificate_credentials>> buildClientCredentials() noexcept;
		seastar::sstring _certificate;
		seastar::sstring _key;
		seastar::sstring _trusted;
		seastar::tls::credentials_builder _builder;
};

}
