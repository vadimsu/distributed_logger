
#include <string>
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

int main(int argc, char **argv){
	uint64_t time_to_run_sec = 60;
	using MyLogger = Logger<PosixBuffer, PosixIO>;
	uint64_t shard = 3;
	string host = __FILE__;
	string certificate;
        string key;
       	string trusted;
	std::cout<<"host "<<host<<std::endl;
	string logserverhost = "127.0.0.1";
	uint16_t logserverport = 7777;
	int rc = get_common_options(argc, argv, time_to_run_sec, certificate, key, trusted, logserverhost, logserverport);
	if (rc != 0){
		return rc;
	}
	shared_ptr<PosixIO> eventPosix = make_shared<PosixIO>(logserverhost,logserverport, std::forward<string>(certificate), std::forward<string>(key), std::forward<string>(trusted));
	shared_ptr<MyLogger> distributedLogger = make_shared<MyLogger>(eventPosix);
	uint64_t start_ts = time(NULL);
	uint64_t logs_submitted = 0;
	while(time(NULL) - start_ts < time_to_run_sec){
		distributedLogger->logEvent(MyLogger::Events::event0, shard, host);
		distributedLogger->logEvent(MyLogger::Events::event1, shard, host, time(NULL));
		logs_submitted += 2;
	}
	cout<<"logs submitted "<<logs_submitted<<endl;
	sleep(10);
	return 0;
}
