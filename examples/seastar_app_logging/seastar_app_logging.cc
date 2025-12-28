
#include <time.h>
#include <seastar/core/app-template.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/sleep.hh>
#include <seastar/core/thread.hh>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include "seastar_iio.hh"
#include "seastar_buffer.hh"
#include "LogAPIs.hh"

namespace bpo = boost::program_options;
using MyLogger = DistributedLogger::Logger<DistributedLogger::EventBufferSeastar, DistributedLogger::Seastar_IIO>;

int main(int argc, char** argv) {
	seastar::app_template app;
	app.add_options()
		("ip", bpo::value<seastar::sstring>()->default_value({}), "Log server's IP address")
		("port", bpo::value<int>()->default_value({}), "Log server's port");
	app.run(argc, argv, [&app] {
		auto& args = app.configuration();
		fmt::print("{} {}\n",args["ip"].as<seastar::sstring>(),args["port"].as<int>());
		seastar::sstring ip = args["ip"].as<seastar::sstring>();
		auto port = args["port"].as<int>();
		auto iio = std::make_shared<DistributedLogger::Seastar_IIO>(ip, port);
		iio->Connect();
		auto logger = std::make_shared<MyLogger>(iio);
		return seastar::async([iio, logger]{
			while(true){
				seastar::sleep(std::chrono::seconds(1)).get();
				if (!iio->isConnected()){
					continue;
				}
				auto shard = 1;
				std::string host = "MySeastarClientHost";
				logger->LogEvent(MyLogger::Events::event0, shard, host);
				shard = 3;
				logger->LogEvent(MyLogger::Events::event1, shard, host, time(NULL));
			}
		});
	});
}
