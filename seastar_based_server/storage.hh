
#pragma once

#include <seastar/core/seastar.hh>

namespace DistributedLogger {
	class Storage : public std::enable_shared_from_this<Storage>{
		public:
			static std::shared_ptr<Storage> Init(const std::unordered_map<seastar::sstring, seastar::sstring>& params);
			virtual seastar::future<> close() = 0;
			virtual seastar::future<> Flush(std::vector<seastar::temporary_buffer<char>>&& batch) = 0;
		private:
	};
}
