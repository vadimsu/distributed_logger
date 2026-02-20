
#include <tuple>
#include <time.h>
#include <seastar/core/app-template.hh>
#include <seastar/core/smp.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/sleep.hh>
#include <seastar/core/thread.hh>
#include <seastar/core/when_all.hh>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include "SeastarIO.hh"
#include "SeastarTLS_IO.hh"
#include "SeastarBuffer.hh"
#include "LogAPIs.hh"
#include "common.hh"

namespace bpo = boost::program_options;
using MyLogger = distributed_logger::Logger<distributed_logger::SeastarBuffer, distributed_logger::SeastarIO>;

static seastar::future<std::tuple<uint64_t,uint64_t,uint64_t>>
run_loop(const LoopParams& loop_params){
	std::shared_ptr<distributed_logger::SeastarIO> iio;
	if (loop_params.certificate != "" && loop_params.key != ""){
		iio = std::make_shared<distributed_logger::SeastarTLS_IO>(loop_params.logserverhost,
				loop_params.logserverport, loop_params.certificate, loop_params.key,
				loop_params.trusted);
	}else{
		iio = std::make_shared<distributed_logger::SeastarIO>(loop_params.logserverhost,
				loop_params.logserverport);
	}
	iio->setQueueMaxSize(loop_params.queuesize);
	iio->connect();
	auto logger = std::make_shared<MyLogger>(iio);
	return seastar::async([iio, logger, loop_params]{
		while(!iio->isConnected()){
			fmt::print("shard {} waiting for connection to be established\n",seastar::this_shard_id());
			seastar::sleep(std::chrono::seconds(1)).get();
		}
		fmt::print("shard {} connection is established\n",seastar::this_shard_id());
		uint64_t start_ts = time(NULL);
		while(time(NULL) - start_ts < loop_params.time_to_run_sec){
			if (!iio->isConnected()){
				fmt::print("shard {} unexpectedly disconnected\n",seastar::this_shard_id());
				continue;
			}
			auto shard = seastar::this_shard_id();
			std::string host = "MySeastarClientHost";
			logger->LogEvent_event0(shard, host);
			logger->LogEvent_event1(shard, host, time(NULL));
			seastar::thread::yield();
		}
		iio->disconnect().get();
		fmt::print("shard {} finished\n",seastar::this_shard_id());
		return std::tuple<uint64_t,uint64_t,uint64_t>(iio->getLogsPostedCount(),
				iio->getLogsDroppedCount(),iio->getLogsSentCount());
	});
}

int main(int argc, char** argv) {
	seastar::app_template app;
	app.add_options()
		("time", bpo::value<uint64_t>()->default_value({60}), "Time to run in seconds")
		("certificate", bpo::value<seastar::sstring>()->default_value({}), "TLS certificate file path")
		("key", bpo::value<seastar::sstring>()->default_value({}), "TLS key file path")
		("trusted", bpo::value<seastar::sstring>()->default_value({}), "TLS root certificate file path (for self-signed)")
		("host", bpo::value<seastar::sstring>()->default_value({}), "Log server's IP address")
		("port", bpo::value<int>()->default_value({}), "Log server's port")
		("size", bpo::value<size_t>()->default_value({}), "Logging queue max size in bytes. 0 means unlimited");
	return app.run_deprecated(argc, argv, [&app] {
		fmt::print("starting on CPU {}\n",seastar::this_shard_id());
		auto& args = app.configuration();
		LoopParams loop_params;
		const bpo::variables_map& config = app.configuration();
		if (config.count("time")){
			fmt::print("{}\n",args["time"].as<uint64_t>());
			loop_params.time_to_run_sec = args["time"].as<uint64_t>();
		}
		if (config.count("host")){
			fmt::print("{}\n",args["host"].as<seastar::sstring>());
			loop_params.logserverhost = args["host"].as<seastar::sstring>();
		}
		if (config.count("port")){
			fmt::print("{}\n",args["port"].as<int>());
			loop_params.logserverport = args["port"].as<int>();
		}
		if (config.count("certificate")){
			fmt::print("{}\n",args["certificate"].as<seastar::sstring>());
			loop_params.certificate = args["certificate"].as<seastar::sstring>();
		}
		if (config.count("key")){
			fmt::print("{}\n",args["key"].as<seastar::sstring>());
			loop_params.key = args["key"].as<seastar::sstring>();
		}
		if (config.count("trusted")){
			fmt::print("{}\n",args["trusted"].as<seastar::sstring>());
			loop_params.trusted = args["trusted"].as<seastar::sstring>();
		}
		if (config.count("size")){
			fmt::print("{}\n",args["size"].as<size_t>());
			loop_params.queuesize = args["size"].as<size_t>();
		}
		try {
			return seastar::async([loop_params]{
				std::vector<seastar::future<std::tuple<uint64_t,uint64_t,uint64_t>>> futs;
				seastar::smp::invoke_on_others([&loop_params, &futs] () mutable{
					auto fut = run_loop(loop_params);
					futs.push_back(std::move(fut));
				}).get();
				seastar::when_all(futs.begin(), futs.end()).then([](auto tups){
					uint64_t logs_submitted_total = 0;
					uint64_t logs_dropped_total = 0;
					uint64_t logs_sent_total = 0;
					for(auto& f : tups){
						auto v = f.get();
						logs_submitted_total += std::get<0>(v);
						logs_dropped_total += std::get<1>(v);
						logs_sent_total += std::get<2>(v);
					}
					fmt::print("Posted {} dropped {} sent {}\n",
							logs_submitted_total, logs_dropped_total,logs_sent_total);
					return seastar::make_ready_future<>();
				}).handle_exception([](std::exception_ptr e){
					fmt::print("{}\n",e);
					return seastar::make_ready_future<>();
				}).get();
			}).then([] {
				// Force a jump back to 0 if we've drifted
				return seastar::smp::submit_to(0, [] {
					seastar::engine().exit(0);
					return seastar::make_ready_future<>();
				});
			});
		}catch(...){
		}
	});
}
