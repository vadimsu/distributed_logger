#pragma once

#include <memory>
#include "IBuffer.hh"

namespace distributed_logger {

class IIO{
        public:
                virtual ~IIO() noexcept {}
                virtual std::shared_ptr<IBufferWrapper> send(std::shared_ptr<IBufferWrapper>) noexcept = 0;
		virtual void setQueueMaxSize(size_t) noexcept = 0;
		virtual size_t QueueSize() noexcept = 0;
		virtual uint64_t getLogsPostedCount() noexcept = 0;
		virtual uint64_t getLogsDroppedCount() noexcept = 0;
		virtual uint64_t getLogsSentCount() noexcept = 0;
};

}
