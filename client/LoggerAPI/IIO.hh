#pragma once

#include <memory>
#include "IBuffer.hh"

namespace DistributedLogger {

class IIO{
        public:
                virtual ~IIO(){}
                virtual std::shared_ptr<IBufferWrapper> Send(std::shared_ptr<IBufferWrapper>) = 0;
};

}
