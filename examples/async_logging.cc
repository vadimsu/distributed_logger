
#include <string>
#include <utility>
#include <time.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <iostream>
#include "EventIIOPosix.hh"
#include "EventBufferPosix.hh"
#include "LogAPIs.hh"


using namespace std;
using namespace DistributedLogger;

int main(int argc, char **argv){
	uint64_t time_to_run_sec = 60;
	using MyLogger = Logger<EventBufferPosix, EventIIOPosix>;
	uint64_t shard = 3;
	string host = __FILE__;
	string certificate;
        string key;
       	string trusted;
	#define MAX_EVENTS 10
        struct epoll_event ev, events[MAX_EVENTS];
	std::cout<<"host "<<host<<std::endl;
	shared_ptr<EventIIOPosix> eventPosix = make_shared<EventIIOPosix>("127.0.0.1",7777, std::forward<string>(certificate), std::forward<string>(key), std::forward<string>(trusted));
	eventPosix->setAsyncMode(true);
	shared_ptr<MyLogger> distributedLogger = make_shared<MyLogger>(eventPosix);
	int epollfd = epoll_create1(0);
	ev.events = EPOLLOUT;
        ev.data.fd = eventPosix->getFd();
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, eventPosix->getFd(), &ev) == -1) {
		cout<<"epoll_ctl: listen_sock"<<endl;
		return -1;
	}
	if (argc > 1){
		time_to_run_sec = stoi(argv[1]);
	}
	uint64_t start_ts = time(NULL);
	uint64_t last_log_ts = time(NULL);
	uint64_t logs_submitted = 0;
	while(time(NULL) - start_ts < time_to_run_sec){
		auto nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		for (int n = 0; n < nfds; ++n) {
			if (events[n].data.fd == eventPosix->getFd()){
				if (eventPosix->isQueueEmpty()){
					distributedLogger->LogEvent(MyLogger::Events::event0, shard, host);
					distributedLogger->LogEvent(MyLogger::Events::event1, shard, host, time(NULL));
					logs_submitted += 2;
				}
				eventPosix->OnWriteOpportunity();
			}	
		}
		if (time(NULL) - last_log_ts > 3){
			distributedLogger->LogEvent(MyLogger::Events::event0, shard, host);
			distributedLogger->LogEvent(MyLogger::Events::event1, shard, host, time(NULL));
			logs_submitted += 2;
			last_log_ts = time(NULL);
		}
	}
	cout<<"logs submitted "<<logs_submitted<<endl;
	sleep(10);
	return 0;
}
