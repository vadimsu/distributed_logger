#include "EventIIOPosix.hh"
#include <syslog.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

#define CHK_NULL(x) if((x) == NULL) exit(1);
#define CHK_ERR(err, s) if((err) == -1) { perror(s); exit(1); }
#define CHK_SSL(err) if((err) == -1) { ERR_print_errors_fp(stderr); exit(2); }

using namespace DistributedLogger;

void EventIIOPosix::initialize_connection(){
        if (_cert == "" || _key == "") {
                char buf[1024];
                struct hostent host_entry[2];
                struct hostent *result;
                int idx = 0, err = 0;
                syslog(LOG_INFO,"getting IP for host %s for centralized logging",_remoteHost.c_str());
                if (gethostbyname_r(_remoteHost.c_str(), host_entry, buf, sizeof(buf), &result, &err)){
                        syslog(LOG_ERR,"Cannot get host for %s",_remoteHost.c_str());
                        return;
                }
                for(;idx < 2; idx++){
                        if (host_entry[idx].h_addrtype == AF_INET){
                                break;
                        }
                }
                if (idx == 2){
                        syslog(LOG_ERR,"Cannot find AF_INET address for %s",_remoteHost.c_str());
                        return;
                }
                _fd = socket(AF_INET, SOCK_STREAM, 0);
                if (_fd <= 0){
                        close(_fd);
                        _fd = -1;
                        return;
                }
                struct sockaddr_in sin;
                sin.sin_family = AF_INET;
                sin.sin_port = htons(7777);
                sin.sin_addr = *(struct in_addr*)host_entry[idx].h_addr_list[0];

                syslog(LOG_INFO,"Connecting to centralized logging %s",_remoteHost.c_str());
                if (connect(_fd, (const struct sockaddr *)&sin, sizeof(sin))){
                        syslog(LOG_ERR,"Cannot connect to centralized logging %x",sin.sin_addr.s_addr);
                        close(_fd);
                        _fd = -1;
                        return;
                }
		_connected = true;
        }else{
		 int err = 0;
                SSL_CTX   *ctx;
                X509                    *server_cert;
                char                    *str;
                struct sockaddr_in sa;

                SSL_load_error_strings();
                SSLeay_add_ssl_algorithms();
                const SSL_METHOD    *meth = TLSv1_2_client_method();
                ctx = SSL_CTX_new(meth);
                CHK_NULL(ctx);

                if(SSL_CTX_use_certificate_file(ctx, _cert.c_str(), SSL_FILETYPE_PEM) <= 0) {
                        ERR_print_errors_fp(stderr);
                        exit(3);
                }

                if(SSL_CTX_use_PrivateKey_file(ctx, _key.c_str(), SSL_FILETYPE_PEM) <= 0) {
                        ERR_print_errors_fp(stderr);
                        exit(4);
                }

                if(!SSL_CTX_check_private_key(ctx)) {
                        fprintf(stderr, "Private key does not match the certificate public keyn");
                        exit(5);
                }

                CHK_SSL(err);

                _fd = socket(AF_INET, SOCK_STREAM, 0);
                CHK_ERR(_fd, "socket");

                memset(&sa, 0x00, sizeof(sa));
                sa.sin_family = AF_INET;
                sa.sin_addr.s_addr = inet_addr(_remoteHost.c_str());
                sa.sin_port = htons(_port);

                err = connect(_fd, (struct sockaddr*)&sa, sizeof(sa));
                CHK_ERR(err, "connect");

                _ssl = SSL_new(ctx);
                CHK_NULL(_ssl);

                SSL_set_fd(_ssl, _fd);
                err = SSL_connect(_ssl);
                if (err) {
                        syslog(LOG_ERR,"on SSL_connect, error %d",err);
                }

                syslog(LOG_INFO,"SSL connection using %s", SSL_get_cipher(_ssl));

                server_cert = SSL_get_peer_certificate(_ssl);
		CHK_NULL(server_cert);
                syslog(LOG_INFO,"Server certificate:");

                str = X509_NAME_oneline(X509_get_subject_name(server_cert), 0, 0);
                CHK_NULL(str);
                syslog(LOG_INFO,"\t subject: %s", str);
                OPENSSL_free(str);

                str = X509_NAME_oneline(X509_get_issuer_name(server_cert), 0, 0);
                CHK_NULL(str);
                syslog(LOG_INFO,"\t issuer: %s", str);
                OPENSSL_free(str);

                X509_free(server_cert);
		_connected = true;
        }
        int send_buf_size = 1024 * 1024;
        if (setsockopt(_fd, SOL_SOCKET, SO_SNDBUF, &send_buf_size, sizeof(send_buf_size)) < 0) {
        }

        int flags = fcntl(_fd, F_GETFL, 0);
        if (flags < 0) {
        }else{
                // Set non-blocking flag
                if (fcntl(_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
                }
        }
        syslog(LOG_INFO,"Connected to centralized logging %s",_remoteHost.c_str());
}

EventIIOPosix::EventIIOPosix(std::string remoteHost, uint16_t port, std::string&& certificate, std::string&& key, std::string&& trusted): _remoteHost(remoteHost), _port(port), _cert(certificate), _key(key), _trusted(trusted), _fd(-1), _ssl(NULL), _connected(false){
        initialize_connection();
}

EventIIOPosix::~EventIIOPosix(){
        if (_ssl != NULL){
                OPENSSL_free(_ssl);
                _ssl = NULL;
        }
}

std::shared_ptr<IBufferWrapper> EventIIOPosix::Send(std::shared_ptr<IBufferWrapper> buf){
        uint32_t length = buf->getCapacity() - 4;
	std::cout<<"Sending "<<buf->getCapacity()<<std::endl;
        length = htonl(length);
        memcpy(buf->getData(), &length, sizeof(length));
//        length = buf->getCapacity() - 10;
//        length = htonl(length);
//        memcpy(buf->getData() + 6, &length, sizeof(length));
        if (!_queue.empty()){
                _queue.push_back(buf);
                return nullptr;
        }
//repeat:
        int written = 0;
        if (_cert == "" || _key == ""){
                written = write(_fd,buf->getData() + buf->getReadOffset(), buf->getCapacity() - buf->getReadOffset());
        }else{
                written = SSL_write(_ssl, buf->getData() + buf->getReadOffset(), buf->getCapacity() - buf->getReadOffset());
        }
        if (written > 0){
                buf->setReadOffset(buf->getReadOffset() + written);
        }else{
                if (errno != 0){
                        initialize_connection();
                        syslog(LOG_ERR,"Cannot send log message %d %s",written,strerror(errno));
                }
//              goto repeat;
        }
        if (buf->getReadOffset() < buf->getCapacity()){
                _queue.push_back(buf);
        }else{
        }
        return nullptr;
}

void EventIIOPosix::OnPeriodic(){
        while (!_queue.empty()){
//repeat:
                auto buf = _queue.front();
		size_t written = 0;
		if (_cert == "" || _key == ""){
			written = write(_fd,buf->getData() + buf->getReadOffset(), buf->getCapacity() - buf->getReadOffset());
		}else{
	                written = SSL_write(_ssl, buf->getData() + buf->getReadOffset(), buf->getCapacity() - buf->getReadOffset());
		}
                if (written > 0){
                        buf->setReadOffset(buf->getReadOffset() + written);
                }else{
                        if (errno != 0){
                                initialize_connection();
                                syslog(LOG_ERR,"Cannot send log message %d %s",written,strerror(errno));
                        }
//                      goto repeat;
                }
                if (buf->getReadOffset() == buf->getCapacity()){
                        _queue.pop_front();
                }else{
                }
        }
}
