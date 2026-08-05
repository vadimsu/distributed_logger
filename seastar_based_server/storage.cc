

#include "storage.hh"
#include "clickhouse.hh"

namespace DistributedLogger {

std::shared_ptr<Storage> Storage::Init(const std::unordered_map<seastar::sstring, seastar::sstring>& params){
	auto it = params.find("StorageType");
	if (it == params.end()){
		fmt::print("Cannot find StorageType\n");
		return nullptr;
	}
	if (it->second == "clickhouse"){
		fmt::print("clickhouse\n");
		seastar::sstring ip;
		it = params.find("Host");
		if (it == params.end()){
			fmt::print("no Host provided\n");
			return nullptr;
		}
		ip = it->second;
		seastar::sstring port;
		it = params.find("Port");
		if (it == params.end()){
			fmt::print("No Port provided\n");
			return nullptr;
		}
		port = it->second;
		seastar::sstring dbname, username, password;
		it = params.find("Dbname");
		if (it != params.end()){
			dbname = it->second;
		}
		it = params.find("Username");
		if (it != params.end()){
			username = it->second;
		}
		it = params.find("Password");
		if (it != params.end()){
			password = it->second;
		}
		return ClickHouseStorage::Init(ip, port, dbname, username, password);
	}else{
		fmt::print("unknown storage {} \n", it->second);
	}
	return nullptr;
}

}
