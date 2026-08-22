
#include <unordered_map>
#include <seastar/core/app-template.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/seastar.hh>
#include <seastar/core/when_all.hh>
#include <seastar/core/distributed.hh>
#include "config.hh"
#include "tcp_af_helper.hh"
#include "listener.hh"
#include "storage.hh"
#include <iostream>

namespace bpo = boost::program_options;

int main(int argc, char **argv){
	seastar::app_template app;
        app.add_options()
                        ("config", bpo::value<seastar::sstring>()->default_value({}), "path to config file");
        return app.run_deprecated(argc, argv, [&app]{
                auto& args = app.configuration();
		seastar::lw_shared_ptr<DistributedLogger::Config> config = seastar::make_lw_shared<DistributedLogger::Config>(args["config"].as<seastar::sstring>());
		seastar::engine().at_exit([] {
			fmt::print("at_exit\n");
                        return seastar::make_ready_future<>();
                });
		return config->read().then([config]{
				const auto& eventCollectorConfig = config->getEventCollectorConfig();
				std::unordered_map<seastar::sstring, seastar::sstring> storageParams;
				const DistributedLogger::StorageConfig& storageConfig = config->getStorageConfig();
				storageParams.emplace("StorageType", storageConfig.getStorageType());
				storageParams.emplace("Host", storageConfig.getHost());
				storageParams.emplace("Port", storageConfig.getPort());
				storageParams.emplace("Dbname", storageConfig.getDbname());
				storageParams.emplace("DataRetentionPeriod", storageConfig.getDataRetentionPeriod());
				storageParams.emplace("Username", storageConfig.getUsername());
				storageParams.emplace("Password", storageConfig.getPassword());
				storageParams.emplace("Protocol", storageConfig.getProtocol());
				storageParams.emplace("WorkersBufferSize", config->getGeneralConfig().getWorkersBufferSize());
				return DistributedLogger::Storage::globalInit(storageParams).then([storageParams, config, port=eventCollectorConfig.getPort(), ip=eventCollectorConfig.getIp()] mutable{
					seastar::distributed<DistributedLogger::Listener> *listener = new seastar::distributed<DistributedLogger::Listener>();
					return listener->start(ip, atoi(port.c_str())).then([listener, storageParams] {
						listener->invoke_on_all(&DistributedLogger::Listener::setStorageParams, storageParams);
						return listener->invoke_on_all(&DistributedLogger::Listener::listen);
					});
				});
		});
        });
}
