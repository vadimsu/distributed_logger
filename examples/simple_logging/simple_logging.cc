
#include <string>
#include <vector>
#include <utility>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include "PosixIO.hh"
#include "PosixBuffer.hh"
#include "LogAPIs.hh"
#include "common.hh"

using namespace std;
using namespace distributed_logger;

static void*
run_loop(void *arg){
	LoopParams *loop_params = static_cast<LoopParams*>(arg);
	using MyLogger = Logger<PosixBuffer, PosixIO>;
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(loop_params->shard, &cpuset);
	pthread_t current_thread = pthread_self();
	pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
	string host = "simple_logging";
	std::cout<<"host "<<host<<" "<<loop_params->shard<<std::endl;
	shared_ptr<PosixIO> eventPosix = make_shared<PosixIO>(loop_params->logserverhost,loop_params->logserverport,
			std::forward<string>(loop_params->certificate), std::forward<string>(loop_params->key),
			std::forward<string>(loop_params->trusted));
	shared_ptr<MyLogger> distributedLogger = make_shared<MyLogger>(eventPosix);
	uint64_t start_ts = time(NULL);
	eventPosix->setQueueMaxSize(loop_params->queuesize);
	while(time(NULL) - start_ts < loop_params->time_to_run_sec){
		distributedLogger->LogEvent_event0(loop_params->shard, host);
		distributedLogger->LogEvent_event1(loop_params->shard, host, time(NULL));
	}
	loop_params->logs_submitted = eventPosix->getLogsPostedCount();
	loop_params->logs_dropped = eventPosix->getLogsDroppedCount();
	loop_params->logs_sent = eventPosix->getLogsSentCount();
	return NULL;
}

int main(int argc, char **argv){
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
