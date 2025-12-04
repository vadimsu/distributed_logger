#pragma once

#include <memory>

class IBufferWrapper{
        public:
                IBufferWrapper(){}
                virtual ~IBufferWrapper(){}
                IBufferWrapper(const IBufferWrapper&) = delete;
                IBufferWrapper(IBufferWrapper&&) = delete;
                virtual IBufferWrapper& operator=(const IBufferWrapper&) = delete;
                virtual IBufferWrapper& operator=(IBufferWrapper&&) = delete;
                virtual size_t readData(char*, size_t) = 0;
                virtual char *getData() = 0;
                virtual size_t getCapacity() = 0;
                virtual size_t writeData(const char*, size_t) = 0;
                virtual size_t getReadOffset() = 0;
                virtual size_t getWriteOffset() = 0;
                virtual void setReadOffset(size_t offset=0) = 0;
                virtual void setWriteOffset(size_t offset=0) = 0;
};
