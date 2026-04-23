#pragma once

#include "IBuffer.hh"
#include "IIO.hh"
#include <list>
#include <memory>
#include <string>

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


namespace distributed_logger {

class PosixIO: public IIO{
        public:
                PosixIO(std::string remoteHost, uint16_t port, std::string&& certificate, std::string&& key, std::string&& trusted) noexcept;
                ~PosixIO();
                std::shared_ptr<IBufferWrapper> send(std::shared_ptr<IBufferWrapper>) noexcept override;
		void setQueueMaxSize(size_t queuemaxsize) noexcept override { _queuemaxsize = queuemaxsize; }
		virtual size_t QueueSize() noexcept override { return _queuesize; }
		uint64_t getLogsPostedCount() noexcept override { return _logPostedCnt; }
		uint64_t getLogsDroppedCount() noexcept override { return _logDroppedCnt; }
		uint64_t getLogsSentCount() noexcept override { return _logSentCnt; }
		void connectionGracefulShutdown() noexcept override;
                void onWriteOpportunity() noexcept;
		int getFd() noexcept { return _fd; }
		bool isConnected() noexcept { return _connected; }
		bool isQueueEmpty() noexcept { return _queue.empty(); }
		bool getAsyncMode() noexcept { return _asyncMode; }
		void setAsyncMode(bool) noexcept;
        private:
		void fixAsyncMode()noexcept;
                void initialize_connection() noexcept;
                std::string _remoteHost;
                uint16_t _port;
                std::string _cert;
                std::string _key;
                std::string _trusted;
                int _fd;
                SSL *_ssl;
                std::list<std::shared_ptr<IBufferWrapper>> _queue;
		bool _connected;
		bool _asyncMode;
		size_t _queuemaxsize;
		size_t _queuesize;
		uint64_t _logPostedCnt;
		uint64_t _logDroppedCnt;
		uint64_t _logSentCnt;
};

}
