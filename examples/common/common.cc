
#include <string>
#include <iostream>
#include <boost/program_options.hpp>
#include "common.hh"

using namespace std;
namespace po = boost::program_options;

int get_common_options(int argc, char** argv, uint64_t& time_to_run_sec, string& certificate, string& key, string& trusted, string& host, uint16_t& port){
	try {
		po::options_description desc("Allowed options");
		desc.add_options()
			("help", "produce help message")
			("time", po::value<int>()->default_value(60), "Time to run in seconds")
			("certificate", po::value<string>()->default_value(""), "TLS certificate file path")
			("key", po::value<string>()->default_value(""), "TLS key file path")
			("trusted", po::value<string>()->default_value(""), "TLS root certificate file path (for self-signed)")
			("host", po::value<string>()->default_value("127.0.0.1"), "Log server's IP address")
			("port", po::value<uint16_t>()->default_value(7777), "Log server's port");
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
		if (vm.count("help")) {
			cout << desc << "\n";
			return 2;
		}
		if (vm.count("time")) {
			time_to_run_sec = vm["time"].as<int>();
		}
		if (vm.count("certificate")){
			certificate = vm["certificate"].as<string>();
		}
		if (vm.count("key")){
			key = vm["key"].as<string>();
		}
		if (vm.count("trusted")){
			trusted = vm["trusted"].as<string>();
		}
		if (vm.count("host")){
			trusted = vm["host"].as<string>();
		}
		if (vm.count("port")){
			port = vm["port"].as<uint16_t>();
		}
	}catch(std::exception& e){
		cerr << "error: " << e.what() <<endl;
		return 2;
	}
	return 0;
}
