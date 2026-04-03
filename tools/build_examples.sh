#!/bin/bash
cd examples/simple_logging
cmake .
make
cd ../async_logging
cmake .
make
cd ../seastar_app_logging
cmake .
make
