#pragma once

#include <memory>
#include "IBuffer.hh"
#include "IIO.hh"

namespace DistributedLogger {

template<typename Buffer, typename IIO>
class Logger{
	public:
		Logger(std::shared_ptr<IIO> iio): _iio(iio) {}
	#include "distributed_logger_api_int.hh"
	private:
		typedef enum {
			INT=1,
			STRING=2
		}LoggingDataType;
		int encode(std::shared_ptr<IBufferWrapper> buffer, const uint64_t& v) {
//                        uint16_t type = static_cast<std::underlying_type_t<LoggingDataType>>(LoggingDataType::INT);
//                        auto written = buffer->writeData((const char*)&type, sizeof(type));
                        auto written = buffer->writeData((const char*)&v, sizeof(v));
                        return written;
                }
		int encode(std::shared_ptr<IBufferWrapper> buffer, const std::string& str) {
                        uint16_t type = static_cast<std::underlying_type_t<LoggingDataType>>(LoggingDataType::STRING);
                        auto written = buffer->writeData((const char*)&type, sizeof(type));
                        uint16_t size = static_cast<uint16_t>(str.size());
                        written += buffer->writeData((const char*)&size, sizeof(size));
                        written += buffer->writeData((const char*)str.data(), str.size());
                        return written;
                }
		std::shared_ptr<IIO> _iio;
};

}
