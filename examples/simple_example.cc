#include <unistd.h>
#include <string>
#include <iostream>
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
		cout<<"Should not reach this point, an unknown argument might be encountered"<<endl;
	}
	gethostname(hostname, sizeof(hostname));
	localhost = string(hostname);
	shared_ptr<EventIIO> iio = make_shared<EventIIO>(remote_host, port, move(tls_certificate), move(tls_key), move(trusted));
	shared_ptr<Logging<EventBuffer, EventIIO>> logging = make_shared<Logging<EventBuffer, EventIIO>>(localhost, iio);
	string zoneuid = "zoneuid1";
	int serial = 2;
	int shard = 3;
	auto buf = logging->LogEvent(1/*event*/, zoneuid, serial, shard, std::make_tuple());
	return 0;
}

