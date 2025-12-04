
add_executable(posix_logger_simple_client)

target_include_directories(posix_logger_simple_client
  PRIVATE
  ../client/iio
  ../client/buffer
  ../client/LoggerAPI
)

target_link_libraries(posix_logger_simple_client
  PRIVATE
  ssl
  crypto
  stdc++
)

target_sources(posix_logger_simple_client
  PRIVATE
  simple_example.cc
  ../client/iio/posix.cc
  ../client/buffer/EventBuffer.cc
)

add_executable(posix_logger_select_client)

target_include_directories(posix_logger_select_client
  PRIVATE
  ../client/iio
  ../client/buffer
  ../client/LoggerAPI
)

target_link_libraries(posix_logger_select_client
  PRIVATE
  ssl
  crypto
  stdc++
)

target_sources(posix_logger_select_client
  PRIVATE
  select_example.cc
  ../client/iio/posix.cc
  ../client/buffer/EventBuffer.cc
)
