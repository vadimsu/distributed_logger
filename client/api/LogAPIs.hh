#pragma once

#include <memory>
#include "IBuffer.hh"
#include "IIO.hh"

namespace distributed_logger {

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
			std::cout<<" writing int "<<sizeof(v)<<std::endl;
			uint64_t v_e = htobe64(v);
                        auto written = buffer->writeData((const char*)&v_e, sizeof(v_e));
                        return written;
                }
		int encode(std::shared_ptr<IBufferWrapper> buffer, const std::string& str) {
                        uint16_t size = htons(static_cast<uint16_t>(str.size()));
			std::cout<<" writing size "<<sizeof(size)<<std::endl;
                        auto written = buffer->writeData((const char*)&size, sizeof(size));
			std::cout<<" writing data "<<str.size()<<std::endl;
                        written += buffer->writeData((const char*)str.data(), str.size());
                        return written;
                }
		std::shared_ptr<IIO> _iio;
};

}
