
#include <string>
#include <iostream>
#include <boost/program_options.hpp>
#include "common.hh"

using namespace std;
namespace po = boost::program_options;

int get_common_options(int argc, char** argv, LoopParams& loop_params){
	try {
		po::options_description desc("Allowed options");
		desc.add_options()
			("help", "produce help message")
			("time", po::value<int>()->default_value(60), "Time to run in seconds")
			("certificate", po::value<string>()->default_value(""), "TLS certificate file path")
			("key", po::value<string>()->default_value(""), "TLS key file path")
			("trusted", po::value<string>()->default_value(""), "TLS root certificate file path (for self-signed)")
			("host", po::value<string>()->default_value("127.0.0.1"), "Log server's IP address")
			("port", po::value<uint16_t>()->default_value(7777), "Log server's port")
			("size", po::value<size_t>()->default_value(0), "Logging queue size (in bytes). 0 means unlimited");
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
		if (vm.count("help")) {
			cout << desc << "\n";
			return 2;
		}
		if (vm.count("time")) {
			loop_params.time_to_run_sec = vm["time"].as<int>();
		}
		if (vm.count("certificate")){
			loop_params.certificate = vm["certificate"].as<string>();
		}
		if (vm.count("key")){
			loop_params.key = vm["key"].as<string>();
		}
		if (vm.count("trusted")){
			loop_params.trusted = vm["trusted"].as<string>();
		}
		if (vm.count("host")){
			loop_params.logserverhost = vm["host"].as<string>();
		}
		if (vm.count("port")){
			loop_params.logserverport = vm["port"].as<uint16_t>();
		}
		if (vm.count("size")){
			loop_params.queuesize = vm["size"].as<size_t>();
		}
	}catch(std::exception& e){
		cerr << "error: " << e.what() <<endl;
		return 2;
	}
	return 0;
}
