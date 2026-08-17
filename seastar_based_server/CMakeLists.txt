cmake_minimum_required(VERSION 3.20)
project(seastar_server)
#set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)
#find_package (Seastar REQUIRED)
#find_package (Valgrind)

include_directories(
	../common
	../../generated/seastar_server
	../generated/seastar_based_server/event_decoder
	../generated/seastar_based_server/storage
	json/include
	../generated/seastar_based_server/storage/clickhouse
)
link_directories(
	seastar_server
	mongo-cxx-driver/src/bsoncxx
)

find_package (Seastar REQUIRED)

add_executable (
        seastar_server
        main.cc
	storage.cc
)

target_link_libraries (
	seastar_server
	PRIVATE Seastar::seastar
	crypto
	ssl
	bsoncxx
)
set(CMAKE_CXX_FLAGS "-O0 -ggdb")
set_target_properties(seastar_server
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
