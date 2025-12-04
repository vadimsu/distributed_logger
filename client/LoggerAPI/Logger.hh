#pragma once

#include<vector>
#include<unordered_map>
#include<memory>

namespace DistributedLogger {

const uint16_t DistributedEventLogVersion = 1;

enum class LoggingDataType {
        EMPTY = 0,
        STRING = 1,
        INT
};

class LoggingType{
        public:
                LoggingType(){}
                virtual ~LoggingType() {}
                virtual int encode(std::shared_ptr<IBufferWrapper> buf){
                        return 0;
                }
                virtual size_t size(){
                        return 0;
                }
                virtual bool operator !=(const LoggingType& rhs) {
                        return false;
                }
};
class LoggingString : public LoggingType{
        public:
                LoggingString(const std::string& str) {
                        _str = str;
                }
                LoggingString(std::string&& str) {
                        _str = std::move(str);
                }
                LoggingString() = delete;
                bool operator !=(const LoggingType& rhs) override{
                        return _str.find(static_cast<const LoggingString&>(rhs)._str) == std::string::npos;
                }
                int encode(std::shared_ptr<IBufferWrapper> buffer) override{
                        uint16_t type = static_cast<std::underlying_type_t<LoggingDataType>>(LoggingDataType::STRING);
                        auto written = buffer->writeData((const char*)&type, sizeof(type));
                        uint16_t size = static_cast<uint16_t>(_str.size());
                        written += buffer->writeData((const char*)&size, sizeof(size));
                        written += buffer->writeData((const char*)_str.data(), _str.size());
                        return written;
                }
                size_t size() override { return _str.size() + sizeof(uint16_t)*2; }
                const std::string& get() const{
                        return _str;
                }
        private:
                std::string _str;
};
class LoggingInt : public LoggingType{
        public:
                LoggingInt(uint64_t v) { _v = v; }
                LoggingInt() = delete;
                bool operator !=(const LoggingType& rhs) override{
                        return _v != static_cast<const LoggingInt&>(rhs)._v;
                }
                int encode(std::shared_ptr<IBufferWrapper> buffer) override{
                        uint16_t type = static_cast<std::underlying_type_t<LoggingDataType>>(LoggingDataType::INT);
                        auto written = buffer->writeData((const char*)&type, sizeof(type));
                        written += buffer->writeData((const char*)&_v, sizeof(_v));
                        return written;
                }
                size_t size() override { return sizeof(_v) + sizeof(uint16_t); }
                uint64_t get(){ return _v; }
        private:
                uint64_t _v;
};

template <typename Buffer, typename IIO>
class Logging {
        public:
                Logging(std::string localhost, std::shared_ptr<IIO> remoteIio): _localhost(std::tuple<LoggingString>(LoggingString(localhost))), _remoteIio(remoteIio) {
                }
                ~Logging(){}
                template <typename... Args>
                std::shared_ptr<IBufferWrapper> LogEvent(uint16_t e, const std::string& zoneuid, int serial, int shard, std::tuple<Args...>&& args){
                        struct timespec tspec;
                        memset(&tspec, 0, sizeof(tspec));
                        uint64_t time_nsec = 0;
                        if (clock_gettime(CLOCK_REALTIME, &tspec) == 0){
                                time_nsec = tspec.tv_sec * 1000000000 + tspec.tv_nsec;
                        }
                        auto now = LoggingInt(time_nsec);
                        size_t total_size = sizeof(uint16_t)*2;//for version & event
                        total_size += getElementSize(args);
                        auto zoneuid_t = std::make_tuple(std::move(LoggingString(zoneuid)));
                        auto serial_t = std::make_tuple(std::move(LoggingInt(serial)));
                        auto shard_t = std::make_tuple(std::move(LoggingInt(shard)));
                        auto timet = std::make_tuple(now);
                        total_size += getElementSize(zoneuid_t);
                        total_size += getElementSize(serial_t);
                        total_size += getElementSize(shard_t);
                        total_size += getElementSize(timet);
                        total_size += getElementSize(_localhost);
                        auto buf = std::make_shared<Buffer>(total_size);
                        buf->writeData((const char*)&DistributedEventLogVersion, sizeof(DistributedEventLogVersion));
//                        uint16_t event = static_cast<std::underlying_type_t<FileSyncEvents>>(e);
			uint16_t event = e;
                        buf->writeData((const char*)&event, sizeof(uint16_t));
                        now.encode(buf);
                        std::get<0>(_localhost).encode(buf);
                        std::get<0>(zoneuid_t).encode(buf);
                        std::get<0>(serial_t).encode(buf);
                        std::get<0>(shard_t).encode(buf);
                        encodeElement(buf, args);
                        return OnWriteEvent(buf);
                }
                std::shared_ptr<IBufferWrapper> OnWriteEvent(std::shared_ptr<Buffer> buf){
                        std::shared_ptr<IBufferWrapper> ret = nullptr;
                        if (_remoteIio){
                                ret = _remoteIio->Send(buf);
                        }
                        return ret;
                }
		std::tuple<size_t, std::tuple<uint16_t, uint16_t, time_t, std::string, std::string, std::vector<std::shared_ptr<LoggingType>>>> decodeEvent(const char* data, size_t size){
                        std::tuple<uint16_t, uint16_t, time_t, std::string, std::string, std::vector<std::shared_ptr<LoggingType>>> event;
                        uint16_t *pVersion = (uint16_t*)data;
                        size_t decoded = sizeof(uint16_t);
                        data += sizeof(uint16_t);
                        std::get<0>(event) = *pVersion;

                        uint16_t *pEvent = (uint16_t*)data;
                        decoded += sizeof(uint16_t);
                        data += sizeof(uint16_t);
                        std::get<1>(event) = *pEvent;

                        uint16_t *pType = (uint16_t *)data;
                        if (*pType != static_cast<uint16_t>(LoggingDataType::INT)){
                                return { decoded, event };
                        }
                        decoded += sizeof(uint16_t);
                        data += sizeof(uint16_t);
                        uint64_t *pNow = (uint64_t *)data;
                        decoded += sizeof(uint64_t);
                        data += sizeof(uint64_t);
                        std::get<2>(event) = *pNow;

                        pType = (uint16_t*)data;
                        if (*pType != static_cast<uint16_t>(LoggingDataType::STRING)){
                                return { decoded, event };
                        }
                        decoded += sizeof(uint16_t);
                        data += sizeof(uint16_t);
                        uint16_t *pLen = (uint16_t *)data;
                        decoded += sizeof(uint16_t);
                        data += sizeof(uint16_t);
                        std::get<3>(event) = std::string(data, *pLen);//localhost
                        decoded += *pLen;
                        data += *pLen;

                        pType = (uint16_t*)data;
                        if (*pType != static_cast<uint16_t>(LoggingDataType::STRING)){
                                return { decoded, event };
                        }
                        decoded += sizeof(uint16_t);
                        data += sizeof(uint16_t);
                        pLen = (uint16_t *)data;
                        decoded += sizeof(uint16_t);
                        data += sizeof(uint16_t);
                        std::get<4>(event) = std::string(data, *pLen);//zoneuid
                        decoded += *pLen;
                        data += *pLen;
			while(decoded < size){
                                pType = (uint16_t *)data;
                                decoded += sizeof(uint16_t);
                                data += sizeof(uint16_t);
                                if (*pType == static_cast<uint16_t>(LoggingDataType::INT)){
                                        uint64_t *pVal = (uint64_t *)data;
                                        decoded += sizeof(uint64_t);
                                        data += sizeof(uint64_t);
                                        auto val = std::make_shared<LoggingInt>(*pVal);
                                        std::get<5>(event).push_back(val);
                                }else if (*pType == static_cast<uint16_t>(LoggingDataType::STRING)){
                                        pLen = (uint16_t*)data;
                                        decoded += sizeof(uint16_t);
                                        data += sizeof(uint16_t);
                                        auto val = std::make_shared<LoggingString>(std::string(data,*pLen));
                                        std::get<5>(event).push_back(val);
                                        decoded += *pLen;
                                        data += *pLen;
                                }else{
                                        break;
                                }
                        }
                        return {decoded, event};
                }
		private:
                using AggregationValue = std::unordered_map<int, std::shared_ptr<IBufferWrapper>>;
                std::list<std::tuple<std::string, std::shared_ptr<IBufferWrapper>>> _queue;
                std::tuple<LoggingString> _localhost;
                std::shared_ptr<IIO> _remoteIio;
                template<size_t I = 0, typename... T>
                        typename std::enable_if<I == sizeof... (T), size_t>::type
                encodeElement(std::shared_ptr<Buffer> p_buf, std::tuple<T...>& t){
                        return 0;
                }

                template<size_t I = 0, typename... T>
                        typename std::enable_if<I < sizeof... (T), size_t>::type
                encodeElement(std::shared_ptr<Buffer> p_buf, std::tuple<T...>& t){
                        auto thisElementLen = std::get<I>(t).encode(p_buf);
                        return thisElementLen + encodeElement<I + 1>(p_buf, t);
                }

                template<size_t I = 0, typename... T>
                        typename std::enable_if<I == sizeof... (T), size_t>::type
                getElementSize(std::tuple<T...>& t){
                        return 0;
                }

                template<size_t I = 0, typename... T>
                        typename std::enable_if<I < sizeof... (T), size_t>::type
                getElementSize(std::tuple<T...>& t){
                        return std::get<I>(t).size() + getElementSize<I + 1>(t);
                }
};

}
