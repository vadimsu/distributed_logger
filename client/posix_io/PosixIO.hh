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
                PosixIO(std::string remoteHost, uint16_t port, std::string&& certificate, std::string&& key, std::string&& trusted);
                ~PosixIO();
                std::shared_ptr<IBufferWrapper> send(std::shared_ptr<IBufferWrapper>) override;
                void onWriteOpportunity();
		int getFd(){ return _fd; }
		bool isConnected() { return _connected; }
		bool isQueueEmpty() { return _queue.empty(); }
		void setAsyncMode(bool mode) { _asyncMode = mode; }
		bool getAsyncMode(){ return _asyncMode; }
        private:
                void initialize_connection();
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
};

}
