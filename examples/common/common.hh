
#pragma once
#include <string>
class LoopParams{
public:
	LoopParams(){
		logserverhost = "127.0.0.1";
		logserverport = 7777;
		certificate = "";
		key = "";
		trusted = "";
		queuesize = 0;
		time_to_run_sec = 60;
		shard = 0;
		logs_submitted = 0;
		logs_dropped = 0;
		logs_sent = 0;
	}
	LoopParams(const LoopParams& other){
		logserverhost = other.logserverhost;
		logserverport = other.logserverport;
		certificate = other.certificate;
		key = other.key;
		trusted = other.trusted;
		queuesize = other.queuesize;
		time_to_run_sec = other.time_to_run_sec;
		shard = other.shard;
	}
	std::string logserverhost;
	uint16_t logserverport;
	std::string certificate;
	std::string key;
	std::string trusted;
	int queuesize;
	uint64_t time_to_run_sec;
	int shard;
	//stats
	uint64_t logs_submitted;
	uint64_t logs_dropped;
	uint64_t logs_sent;
};

int get_common_options(int argc, char** argv,LoopParams& loop_params);
