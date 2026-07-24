#pragma once

#include <vector>
#include "nlohmann/json.hpp"
#include "seastar/core/file.hh"
#include "seastar/core/thread.hh"

namespace fs = std::filesystem;

namespace DistributedLogger {
	class ConfigReader {
		public:
			seastar::future<> readConfig(const seastar::sstring& fileName){
				fs::path config_filename(fileName);
			        return seastar::open_file_dma(config_filename.string(), seastar::open_flags::ro).then([this] (seastar::file f) {
                        		return do_with(std::move(f), [this] (seastar::file& f) {
                                        	return f.size().then([this, &f] (size_t s) {
                                                        return f.dma_read_exactly<char>(0, s);
                                        });
	                        }).then([this] (seastar::temporary_buffer<char> tb) {
					nlohmann::json	jsonPayload;
					seastar::sstring output(tb.get());
                        	        try {
                                	        jsonPayload = nlohmann::json::parse(output);
						onJsonPayload(jsonPayload);
        	                        } catch(std::exception& exc){
						std::cout<<"Exception "<<exc.what()<<std::endl;
                        	        }
        	                        return seastar::make_ready_future<>();
                	                });
                        	}).finally([this]{
					return seastar::make_ready_future<>();
	                        });
			}
		protected:
			virtual void onJsonPayload(nlohmann::json& jsonPayload) = 0;
	};
	class EventCollectorConfig :public ConfigReader{
		public:
			EventCollectorConfig(){}
			void onJsonPayload(nlohmann::json& jsonPayload) override{
				auto it = jsonPayload.find("listener");
				if (it != jsonPayload.end()){
					auto it2 = it->find("ip");
					if (it2 != it->end()){
						_ip = to_string(*it2);
						_ip = _ip.substr(1, _ip.size() - 2);
					}
					it2 = it->find("port");
					if (it2 != it->end()){
						_port = to_string(*it2);
						_port = _port.substr(1, _port.size() - 2);
					}
				}
				it = jsonPayload.find("certificate");
				if (it != jsonPayload.end()){
					auto it2 = it->find("key");
					if (it2 != it->end()){
						_key = to_string(*it2);
						_key = _key.substr(1, _key.size() - 2);
					}
					it2 = it->find("server");
					if (it2 != it->end()){
						_server = to_string(*it2);
						_server = _server.substr(1, _server.size() - 2);
					}
					it2 = it->find("root");
					if (it2 != it->end()){
						_root = to_string(*it2);
						_root = _root.substr(1, _root.size() - 2);
					}
				}
				fmt::print("EventCollector configuration IP {} Port {} cert {} key {} root {}\n",_ip,_port,_server,_key,_root);
			}
			const seastar::sstring& getIp() const { return _ip; }
			const seastar::sstring& getPort() const { return _port; }
			const seastar::sstring& getKey() const { return _key; }
			const seastar::sstring& getServer() const { return _server; }
			const seastar::sstring& getRoot() const { return _root; }
		private:
			seastar::sstring _ip;
			seastar::sstring  _port;
			seastar::sstring _key;
			seastar::sstring _server;
			seastar::sstring _root;
	};
	class StorageConfig :public ConfigReader{
		public:
			StorageConfig(){}
			void onJsonPayload(nlohmann::json& jsonPayload) override{
				fmt::print("Processing StorageConfig configuration\n");
				auto it = jsonPayload.find("StorageType");
				if (it != jsonPayload.end()){
					_storageType = to_string(*it);
					_storageType = _storageType.substr(1, _storageType.size() - 2);
				}
				it = jsonPayload.find("Username");
				if (it != jsonPayload.end()){
					_username = to_string(*it);
					_username = _username.substr(1, _username.size() - 2);
				}
				it = jsonPayload.find("Password");
				if (it != jsonPayload.end()){
					_password = to_string(*it);
					_password = _password.substr(1, _password.size() - 2);
				}
				it = jsonPayload.find("Host");
				if (it != jsonPayload.end()){
					_host = to_string(*it);
					_host = _host.substr(1, _host.size() - 2);
				}
				it = jsonPayload.find("Port");
				if (it != jsonPayload.end()){
					_port = to_string(*it);
					_port = _port.substr(1, _port.size() - 2);
				}
				it = jsonPayload.find("Dbname");
				if (it != jsonPayload.end()){
					_dbname = to_string(*it);
					_dbname = _dbname.substr(1, _dbname.size() - 2);
				}
				it = jsonPayload.find("DataRetentionPeriod");
				if (it != jsonPayload.end()){
					_dataRetentionPeriod = to_string(*it);
					_dataRetentionPeriod = _dataRetentionPeriod.substr(1, _dataRetentionPeriod.size() - 2);
				}
				fmt::print("StorageConfig type {} Username {} Password {} Host {} Port {} Dbname {} DataRetentionPeriod {}\n",_storageType,_username,_password,_host,_port,_dbname,_dataRetentionPeriod);
			}
			const seastar::sstring& getStorageType() { return _storageType; }
			const seastar::sstring& getUsername() { return _username; }
			const seastar::sstring& getPassword() { return _password; }
			const seastar::sstring& getHost() { return _host; }
			const seastar::sstring& getPort() { return _port; }
			const seastar::sstring& getDbname() { return _dbname; }
			const seastar::sstring& getDataRetentionPeriod() { return _dataRetentionPeriod; }
		private:
			seastar::sstring _storageType;
			seastar::sstring  _username;
			seastar::sstring _password;
			seastar::sstring _host;
			seastar::sstring _port;
			seastar::sstring _dbname;
			seastar::sstring _dataRetentionPeriod;
	};
	class GeneralConfig : public ConfigReader {
		public:
			GeneralConfig():_storageConfigFileName(""), _eventCollectorFileName(""),_numberOfWorkers(0),_workersBufferSize(0){}
			void onJsonPayload(nlohmann::json& jsonPayload) override{
				fmt::print("Processing GeneralConfig configuration\n");
				auto it = jsonPayload.find("StorageConfigFileName");
				if (it != jsonPayload.end()){
					_storageConfigFileName = to_string(*it);
					_storageConfigFileName = _storageConfigFileName.substr(1, _storageConfigFileName.size() - 2);
				}else{
					fmt::print("no storage config file name found\n");
				}
				it = jsonPayload.find("EventCollectorFileName");
				if (it != jsonPayload.end()){
					_eventCollectorFileName = to_string(*it);
					_eventCollectorFileName = _eventCollectorFileName.substr(1, _eventCollectorFileName.size() - 2);
				}else{
					fmt::print("no event collector config file name found\n");
				}
				it = jsonPayload.find("NumberOfWorkers");
				if (it != jsonPayload.end()){
					auto numberOfWorkersS = to_string(*it);
					numberOfWorkersS = numberOfWorkersS.substr(1, numberOfWorkersS.size() - 2);
					_numberOfWorkers = atoi(numberOfWorkersS.c_str());
				}
				it = jsonPayload.find("WorkersBufferSize");
				if (it != jsonPayload.end()){
					auto workersBufferSizeS = to_string(*it);
					workersBufferSizeS = workersBufferSizeS.substr(1, workersBufferSizeS.size() - 2);
					_workersBufferSize = atoi(workersBufferSizeS.c_str());
				}
			}
			const seastar::sstring& getStorageConfigFileName() { return _storageConfigFileName; }
			const seastar::sstring& getEventCollectorFileName() { return _eventCollectorFileName; }
		private:
			seastar::sstring _storageConfigFileName;
			seastar::sstring _eventCollectorFileName;
			unsigned _numberOfWorkers;
		        unsigned _workersBufferSize;
	};
	class Config : public seastar::enable_lw_shared_from_this<Config> {
		public:
			Config(const seastar::sstring& json_file): _json_file(json_file){}
			~Config(){fmt::print("{} {}\n",__func__,__LINE__);}
			seastar::future<> read(){
				return _generalConfig.readConfig(_json_file).then([this]{
					auto fut1 = _storageConfig.readConfig(_generalConfig.getStorageConfigFileName());
					auto fut2 = _eventCollectorConfig.readConfig(_generalConfig.getEventCollectorFileName());
					return seastar::when_all(std::move(fut1), std::move(fut2)).then([] (auto futs) {
						return seastar::make_ready_future<>();
					});
				});
			}
			const GeneralConfig& getGeneralConfig() { return _generalConfig; }
			const StorageConfig& getStorageConfig() { return _storageConfig; }
			const EventCollectorConfig& getEventCollectorConfig () { return _eventCollectorConfig; }
		private:
			seastar::sstring _json_file;
			GeneralConfig _generalConfig;
			StorageConfig _storageConfig;
			EventCollectorConfig _eventCollectorConfig;
	};
}
