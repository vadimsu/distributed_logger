#pragma once

namespace DistributedLogger {
	template <typename Buffer, typename IO>
	class DistributedLogger {
		public:
			DistributedLogger(std::shared_ptr<Buffer> buffer, std::shared_ptr<IO> iio): _buffer(buffer), _iio(iio){}
			~DistributedLogger(){}
#include "LogAPIs.hh"
		private:
			int encode(std::shared_ptr<Buffer> buffer, std::string str) {
                        	uint16_t type = static_cast<std::underlying_type_t<LoggingDataType>>(LoggingDataType::STRING);
	                        auto written = buffer->writeData((const char*)&type, sizeof(type));
        	                uint16_t size = static_cast<uint16_t>(str.size());
                	        written += buffer->writeData((const char*)&size, sizeof(size)); 
                        	written += buffer->writeData((const char*)str.data(), str.size());
	                        return written; 
                	}
			int encode(std::shared_ptr<Buffer> buffer, uint64_t v) {
                        	uint16_t type = static_cast<std::underlying_type_t<LoggingDataType>>(LoggingDataType::INT);
	                        auto written = buffer->writeData((const char*)&type, sizeof(type));
        	                written += buffer->writeData((const char*)&v, sizeof(v));
                	        return written;
	                }
			str::shared_ptr<IO> _iio;
	};
}
