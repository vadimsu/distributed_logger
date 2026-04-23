
#include <string>
#include <vector>
#include <utility>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <sys/epoll.h>
#include <iostream>
#include "PosixIO.hh"
#include "PosixBuffer.hh"
#include "LogAPIs.hh"
#include "common.hh"


using namespace std;
using namespace distributed_logger;

void *run_loop(void *arg){
	LoopParams *loop_params = static_cast<LoopParams*>(arg);
	using MyLogger = Logger<PosixBuffer, PosixIO>;
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(loop_params->shard, &cpuset);
	pthread_t current_thread = pthread_self();
	pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
	string host = "async_logging";
	#define MAX_EVENTS 10
        struct epoll_event ev, events[MAX_EVENTS];
	shared_ptr<PosixIO> eventPosix = make_shared<PosixIO>(loop_params->logserverhost, loop_params->logserverport,
			std::forward<string>(loop_params->certificate), std::forward<string>(loop_params->key),
			std::forward<string>(loop_params->trusted));
	eventPosix->setQueueMaxSize(loop_params->queuesize);
	eventPosix->setAsyncMode(true);
	shared_ptr<MyLogger> distributedLogger = make_shared<MyLogger>(eventPosix);
	int epollfd = epoll_create1(0);
	ev.events = EPOLLOUT;
        ev.data.fd = eventPosix->getFd();
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, eventPosix->getFd(), &ev) == -1) {
		cout<<"epoll_ctl: listen_sock"<<endl;
		return NULL;
	}
	uint64_t start_ts = time(NULL);
	uint64_t last_log_ts = time(NULL);
	while(time(NULL) - start_ts < loop_params->time_to_run_sec){
		auto nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		for (int n = 0; n < nfds; ++n) {
			if (events[n].data.fd == eventPosix->getFd()){
				distributedLogger->LogEvent_event0(loop_params->shard, host);
				distributedLogger->LogEvent_event1(loop_params->shard, host, time(NULL));
				eventPosix->onWriteOpportunity();
			}	
		}
	}
	eventPosix->connectionGracefulShutdown();
	loop_params->logs_submitted = eventPosix->getLogsPostedCount();
	loop_params->logs_dropped = eventPosix->getLogsDroppedCount();
	loop_params->logs_sent = eventPosix->getLogsSentCount();
	return NULL;
}

int main(int argc, char **argv){
	uint64_t shard = 3;
	LoopParams loop_params;	
	int rc = get_common_options(argc, argv, loop_params);
	if (rc != 0){
		return rc;
	}
	long num_cores;

	// Get the number of online processors (logical cores)
	num_cores = sysconf(_SC_NPROCESSORS_ONLN);
	vector<tuple<pthread_t,LoopParams*>> threads;
	for (int core = 1; core < num_cores; core++){
		pthread_t threadid;
		LoopParams *loop_params_clone = new LoopParams(loop_params);
		loop_params_clone->shard = core;
		pthread_create(&threadid, NULL, run_loop, loop_params_clone);
		threads.push_back(make_tuple<>(threadid, loop_params_clone));
	}
	uint64_t logs_submitted_total = 0;
	uint64_t logs_dropped_total = 0;
	uint64_t logs_sent_total = 0;
	for(auto t : threads){
		pthread_join(get<0>(t), NULL);
		logs_submitted_total += get<1>(t)->logs_submitted;
		logs_dropped_total += get<1>(t)->logs_dropped;
		logs_sent_total += get<1>(t)->logs_sent;
	}
	cout<<"logs posted "<<logs_submitted_total<<" dropped "<<logs_dropped_total<<endl;
	cout<<"logs sent "<<logs_sent_total<<endl;
	return 0;
}
