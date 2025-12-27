
#include <string>
#include <utility>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include "EventIIOPosix.hh"
#include "EventBufferPosix.hh"
#include "LogAPIs.hh"


using namespace std;
using namespace DistributedLogger;

int main(int argc, char **argv){
	using MyLogger = Logger<EventBufferPosix, EventIIOPosix>;
	uint64_t shard = 3;
	string host = __FILE__;
	string certificate;
        string key;
       	string trusted;
	std::cout<<"host "<<host<<std::endl;
	shared_ptr<EventIIOPosix> eventPosix = make_shared<EventIIOPosix>("127.0.0.1",7777, std::forward<string>(certificate), std::forward<string>(key), std::forward<string>(trusted));
	shared_ptr<MyLogger> distributedLogger = make_shared<MyLogger>(eventPosix);
	distributedLogger->LogEvent(MyLogger::Events::event0, shard, host);
	distributedLogger->LogEvent(MyLogger::Events::event1, shard, host, time(NULL));
	sleep(10);
	return 0;
}
