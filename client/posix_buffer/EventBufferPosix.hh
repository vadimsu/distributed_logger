#pragma once

#include "IBuffer.hh"

namespace DistributedLogger {

class EventBufferPosix : public IBufferWrapper{
        public:
                EventBufferPosix(size_t);
                ~EventBufferPosix();

                size_t readData(char*, size_t)override;
                char *getData() override;
                size_t getCapacity() override;
                size_t writeData(const char*, size_t) override;
                size_t getReadOffset() override;
                size_t getWriteOffset() override;
                void setReadOffset(size_t offset=0) override;
                void setWriteOffset(size_t offset=0) override;
        private:
                char *_buffer;
                size_t _size;
                size_t _readOffset;
                size_t _writeOffset;
};

}
