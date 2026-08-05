
#include <unordered_map>
#include <seastar/core/app-template.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/seastar.hh>
#include <seastar/core/when_all.hh>
#include "config.hh"
#include "tcp_af_helper.hh"
#include "listener.hh"

namespace bpo = boost::program_options;

int main(int argc, char **argv){
	seastar::app_template app;
        app.add_options()
                        ("config", bpo::value<seastar::sstring>()->default_value({}), "path to config file");
        return app.run_deprecated(argc, argv, [&app]{
                auto& args = app.configuration();
                //GdnsFileSync::filesyncapp = new distributed<GdnsFileSync::gdnsfilesync_app>();
		seastar::lw_shared_ptr<DistributedLogger::Config> config = seastar::make_lw_shared<DistributedLogger::Config>(args["config"].as<seastar::sstring>());
		seastar::engine().at_exit([] {
			fmt::print("at_exist\n");
                        return seastar::make_ready_future<>();
                });
		fmt::print("main\n");
		return config->read().then([config]{
				const auto& eventCollectorConfig = config->getEventCollectorConfig();
				std::vector<seastar::future<>> futs;
				std::vector<seastar::lw_shared_ptr<DistributedLogger::Listener>> listeners;
				auto tcpAfHelper = std::make_shared<DistributedLogger::TcpAfHelper>(eventCollectorConfig.getIp(), atoi(eventCollectorConfig.getPort().c_str()));
				auto listener = seastar::make_lw_shared<DistributedLogger::Listener>(tcpAfHelper);
				std::unordered_map<seastar::sstring, seastar::sstring> storageParams;
				const DistributedLogger::StorageConfig& storageConfig = config->getStorageConfig();
				storageParams.emplace("StorageType", storageConfig.getStorageType());
				storageParams.emplace("Host", storageConfig.getHost());
				storageParams.emplace("Port", storageConfig.getPort());
				storageParams.emplace("Dbname", storageConfig.getDbname());
				storageParams.emplace("DataRetentionPeriod", storageConfig.getDataRetentionPeriod());
				storageParams.emplace("Username", storageConfig.getUsername());
				storageParams.emplace("Password", storageConfig.getPassword());
				listener->setStorageParams(storageParams);
				auto fut = listener->listen();
				listeners.push_back(listener);
				futs.push_back(std::move(fut));

				fmt::print("created listener {} {}\n",eventCollectorConfig.getIp(),eventCollectorConfig.getPort());
				return when_all(futs.begin(),futs.end()).then([listeners, config] (auto futs){
						fmt::print("{} {}\n",__FILE__,__LINE__);
						return seastar::make_ready_future<>();
					});
			});
        });
}
