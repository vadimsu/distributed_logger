#include <unistd.h>
#include <string>
#include <iostream>
#include <random>
#include <sys/epoll.h>
#include <stdlib.h>
#include "EventBuffer.hh"
#include "posix.hh"
#include "Logger.hh"

using namespace DistributedLogger;
using namespace std;

int main(int argc, char **argv){
	string tls_key;
	string tls_certificate;
	string trusted;
	string remote_host;
	string localhost;
	char hostname[256];
	int port = 0;
	int i = 1;
	int number_of_logs = 1;
	while(i < argc){
		if (strcmp(argv[i],"--host") == 0){
			remote_host = string(argv[i+1]);
			i += 2;
			continue;
		}
		if (strcmp(argv[i],"--port") == 0){
			port = atoi(argv[i+1]);
			i += 2;
			continue;
		}
		if (strcmp(argv[i],"--key") == 0){
			tls_key = string(argv[i+1]);
			i += 2;
			continue;
		}
		if (strcmp(argv[i],"--cert") == 0){
			tls_certificate = string(argv[i+1]);
			i += 2;
			continue;
		}
		if (strcmp(argv[i],"--trusted") == 0){
			trusted = string(argv[i+1]);
			i += 2;
			continue;
		}
		if (strcmp(argv[i],"--numlogs") == 0){
			number_of_logs = atoi(argv[i+1]);
			i += 2;
			continue;
		}
		cout<<"Should not reach this point, an unknown argument might be encountered"<<endl;
	}
	gethostname(hostname, sizeof(hostname));
	localhost = string(hostname);
	shared_ptr<EventIIO> iio = make_shared<EventIIO>(remote_host, port, move(tls_certificate), move(tls_key), move(trusted));
	shared_ptr<Logging<EventBuffer, EventIIO>> logging = make_shared<Logging<EventBuffer, EventIIO>>(localhost, iio);
	int epollfd = epoll_create(1);
	struct epoll_event event;
	memset(&event, 0, sizeof(event));
	event.data.ptr = nullptr;
	event.events = EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, iio->getFd(), &event)){
	}
	for (i = 0; i < number_of_logs;){
		struct epoll_event events[1];
		auto num_of_events = epoll_wait(epollfd, events, 1, 100);
		if (num_of_events == 0){
			continue;
		}
		if (events[0].events & EPOLLOUT){
			if (iio->isConnected() && iio->isQueueEmpty()){
				int choice = rand() % 2;
				if (choice % 2) {
					char suffix_buffer[124];
					string zoneuid = "zoneuid"+to_string(i+1);
					int serial = 2 + i;
					int shard = 3;
					auto buf = logging->LogEvent(rand() % 10, zoneuid, serial, shard, std::make_tuple());
					std::cout<<"posted log "<<zoneuid<<" "<<serial<<" "<<(buf == nullptr)<<std::endl;
					if (buf == nullptr){
						i++;
					}
				}else{
					iio->OnPeriodic();
				}
			}else{
				sleep(1);
			}
		}
	}
	return 0;
}

