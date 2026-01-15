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
	../../client/seastar_io
	../../client/seastar_buffer
)
link_directories(
)

find_package (Seastar REQUIRED)

add_executable (
        seastar_app_logging
        seastar_app_logging.cc
	../../client/seastar_io/SeastarIO.cc
	../../client/seastar_io/SeastarTLS_IO.cc
	../../client/seastar_buffer/SeastarBuffer.cc
)

target_link_libraries (
	seastar_app_logging
	PRIVATE Seastar::seastar
	crypto
	ssl
)

set_target_properties(seastar_app_logging
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
