
#include <time.h>
#include <seastar/core/app-template.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/sleep.hh>
#include <seastar/core/thread.hh>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include "SeastarIO.hh"
#include "SeastarTLS_IO.hh"
#include "SeastarBuffer.hh"
#include "LogAPIs.hh"

namespace bpo = boost::program_options;
using MyLogger = distributed_logger::Logger<distributed_logger::SeastarBuffer, distributed_logger::SeastarIO>;

int main(int argc, char** argv) {
	seastar::app_template app;
	app.add_options()
		("time", bpo::value<uint64_t>()->default_value({}), "Time to run in seconds")
		("certificate", bpo::value<seastar::sstring>()->default_value({}), "TLS certificate file path")
		("key", bpo::value<seastar::sstring>()->default_value({}), "TLS key file path")
		("trusted", bpo::value<seastar::sstring>()->default_value({}), "TLS root certificate file path (for self-signed)")
		("host", bpo::value<seastar::sstring>()->default_value({}), "Log server's IP address")
		("port", bpo::value<int>()->default_value({}), "Log server's port");
	app.run(argc, argv, [&app] {
		auto& args = app.configuration();
		uint64_t time_to_run_sec = 60;
		seastar::sstring ip = "127.0.0.1";
		auto port = 7777;
		seastar::sstring certificate;
		seastar::sstring key;
		seastar::sstring trusted;
		std::shared_ptr<distributed_logger::SeastarIO> iio;
		const bpo::variables_map& config = app.configuration();
		if (config.count("time")){
			fmt::print("{}\n",args["time"].as<uint64_t>());
			time_to_run_sec = args["time"].as<uint64_t>();
		}
		if (config.count("host")){
			fmt::print("{}\n",args["host"].as<seastar::sstring>());
			ip = args["host"].as<seastar::sstring>();
		}
		if (config.count("port")){
			fmt::print("{}\n",args["port"].as<int>());
			port = args["port"].as<int>();
		}
		if (config.count("certificate")){
			fmt::print("{}\n",args["certificate"].as<seastar::sstring>());
			certificate = args["certificate"].as<seastar::sstring>();
		}
		if (config.count("key")){
			fmt::print("{}\n",args["key"].as<seastar::sstring>());
			key = args["key"].as<seastar::sstring>();
		}
		if (config.count("trusted")){
			fmt::print("{}\n",args["trusted"].as<seastar::sstring>());
			trusted = args["trusted"].as<seastar::sstring>();
		}
		if (certificate != "" && key != ""){
			iio = std::make_shared<distributed_logger::SeastarTLS_IO>(ip, port, certificate, key, trusted);
		}else{
			iio = std::make_shared<distributed_logger::SeastarIO>(ip, port);
		}
		iio->connect();
		auto logger = std::make_shared<MyLogger>(iio);
		return seastar::async([iio, logger, time_to_run_sec]{
			while(!iio->isConnected()){
				fmt::print("waiting for connection to be established\n");
				seastar::sleep(std::chrono::seconds(1)).get();
			}
			uint64_t start_ts = time(NULL);
			uint64_t logs_submitted = 0;
			while(time(NULL) - start_ts < time_to_run_sec){
				seastar::sleep(std::chrono::seconds(1)).get();
				if (!iio->isConnected()){
					continue;
				}
				auto shard = 1;
				std::string host = "MySeastarClientHost";
				logger->LogEvent_event0(shard, host);
				shard = 3;
				logger->LogEvent_event1(shard, host, time(NULL));
			}
			iio->disconnect().get();
		});
	});
}
