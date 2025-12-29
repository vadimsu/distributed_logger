#pragma once

#include <memory>
#include "IBuffer.hh"

namespace distributed_logger {

class IIO{
        public:
                virtual ~IIO(){}
                virtual std::shared_ptr<IBufferWrapper> send(std::shared_ptr<IBufferWrapper>) = 0;
};

}
