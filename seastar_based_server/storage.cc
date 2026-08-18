

#include "storage.hh"
#include "clickhouse.hh"

namespace DistributedLogger {


std::tuple<seastar::sstring, seastar::sstring, seastar::sstring, seastar::sstring, seastar::sstring> getParams(const std::unordered_map<seastar::sstring, seastar::sstring>& params){
	std::tuple<seastar::sstring, seastar::sstring, seastar::sstring, seastar::sstring, seastar::sstring> ret;
	auto it = params.find("StorageType");
	if (it == params.end()){
		fmt::print("Cannot find StorageType\n");
		return ret;
	}
	if (it->second == "clickhouse"){
		fmt::print("clickhouse\n");
		seastar::sstring ip;
		it = params.find("Host");
		if (it == params.end()){
			fmt::print("no Host provided\n");
			return ret;
		}
		std::get<0>(ret) = it->second;
		it = params.find("Port");
		if (it == params.end()){
			fmt::print("No Port provided\n");
			return ret;
		}
		std::get<1>(ret) = it->second;
		it = params.find("Dbname");
		if (it != params.end()){
			std::get<2>(ret) = it->second;
		}
		it = params.find("Username");
		if (it != params.end()){
			std::get<3>(ret) = it->second;
		}
		it = params.find("Password");
		if (it != params.end()){
			std::get<4>(ret) = it->second;
		}
	}else{
		fmt::print("unknown storage {} \n", it->second);
	}
	return ret;
}


std::shared_ptr<Storage> Storage::Init(const std::unordered_map<seastar::sstring, seastar::sstring>& params){
	auto par = getParams(params);
	return ClickHouseStorage::Init(std::get<0>(par), std::get<1>(par), std::get<2>(par), std::get<3>(par), std::get<4>(par));
}

seastar::future<> Storage::globalInit(const std::unordered_map<seastar::sstring, seastar::sstring>& params){
	auto par = getParams(params);
	return ClickHouseStorage::globalInit(std::get<0>(par), std::get<1>(par), std::get<2>(par), std::get<3>(par), std::get<4>(par));
}

}
