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
		SeastarIO(const seastar::sstring&, unsigned int) noexcept;
		virtual ~SeastarIO() noexcept ;
		SeastarIO(const SeastarIO&) = delete;
		SeastarIO(SeastarIO&&) = delete;
		SeastarIO& operator=(const SeastarIO&) = delete;
		SeastarIO& operator=(SeastarIO&&) = delete;
                virtual std::shared_ptr<IBufferWrapper> send(std::shared_ptr<IBufferWrapper>) noexcept override;
		void setQueueMaxSize(size_t queuesize) noexcept override { _queuemaxsize = queuesize; }
		virtual size_t QueueSize() noexcept override { return _queuesize; }
		uint64_t getLogsPostedCount() noexcept override { return _logPostedCnt; }
		uint64_t getLogsDroppedCount() noexcept override { return _logDroppedCnt; }
		uint64_t getLogsSentCount() noexcept override { return _logSentCnt; }
		virtual void connect() noexcept;
		virtual bool isConnected() noexcept { return _connected; }
		virtual seastar::future<> disconnect() noexcept;
	protected:
		virtual void reconnect() noexcept;
		void initializeConnectedSocket(seastar::connected_socket) noexcept;
		seastar::timer<> _reconnect_timer;
		seastar::socket_address _address;
	private:
		seastar::connected_socket _socket;
		seastar::input_stream<char> _in;
		seastar::output_stream<char> _out;
		std::list<seastar::temporary_buffer<char>> _wqueue;
		bool _connected;
		bool _transmitting;
		seastar::future<> _tx_finished;
		size_t _queuemaxsize;
		size_t _queuesize;
		uint64_t _logPostedCnt;
		uint64_t _logDroppedCnt;
		uint64_t _logSentCnt;
		bool _shutting_down;
};

}
