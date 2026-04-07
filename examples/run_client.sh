#!/bin/bash
cd /home/distributed_logger
./tools/run_parser.sh ./examples/example_header.hh
cd examples/async_logging
rm -rf CMakeFiles CMakeCache.txt
cmake .
make
./bin/async_logging --host 172.20.0.4 --port 7777
/bin/bash
