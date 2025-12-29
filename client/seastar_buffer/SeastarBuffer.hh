#pragma once

#include <seastar/core/seastar.hh>
#include <seastar/core/temporary_buffer.hh>
#include "IBuffer.hh"

namespace distributed_logger {

class SeastarBuffer : public IBufferWrapper{
        public:
                SeastarBuffer(size_t);
                ~SeastarBuffer();

                size_t readData(char*, size_t)override;
                char *getData() override;
                size_t getCapacity() override;
                size_t writeData(const char*, size_t) override;
                size_t getReadOffset() override;
                size_t getWriteOffset() override;
                void setReadOffset(size_t offset=0) override;
                void setWriteOffset(size_t offset=0) override;
		seastar::temporary_buffer<char> getBuffer();
        private:
                seastar::temporary_buffer<char> _buffer;
                size_t _readOffset;
                size_t _writeOffset;
};

}
