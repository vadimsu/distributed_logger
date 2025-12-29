
#include <string>
#include <utility>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include "PosixIO.hh"
#include "PosixBuffer.hh"
#include "LogAPIs.hh"


using namespace std;
using namespace distributed_logger;

int main(int argc, char **argv){
	using MyLogger = Logger<PosixBuffer, PosixIO>;
	uint64_t shard = 3;
	string host = __FILE__;
	string certificate;
        string key;
       	string trusted;
	std::cout<<"host "<<host<<std::endl;
	shared_ptr<PosixIO> eventPosix = make_shared<PosixIO>("127.0.0.1",7777, std::forward<string>(certificate), std::forward<string>(key), std::forward<string>(trusted));
	shared_ptr<MyLogger> distributedLogger = make_shared<MyLogger>(eventPosix);
	distributedLogger->logEvent(MyLogger::Events::event0, shard, host);
	distributedLogger->logEvent(MyLogger::Events::event1, shard, host, time(NULL));
	sleep(10);
	return 0;
}
