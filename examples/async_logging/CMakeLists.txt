cmake_minimum_required(VERSION 3.20)
project(examples)
#set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)
#find_package (Seastar REQUIRED)
#find_package (Valgrind)

include_directories(
        ../../client/api
        ../../client/api/iio
        ../../client/api/buffer
	../../client/internal
	../../client/posix_io
	../../client/posix_buffer
)
link_directories(
)

add_executable (
        async_logging
        async_logging.cc
	../../client/posix_io/PosixIO.cc
	../../client/posix_buffer/PosixBuffer.cc
)

target_link_libraries (
	async_logging
	crypto
	ssl
)

set_target_properties(async_logging
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
