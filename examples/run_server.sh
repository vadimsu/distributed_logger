#!/bin/bash
cd /home/distributed_logger
./tools/run_parser.sh ./examples/example_header.hh
cd server/main
go get distributedlogger.com/clickhouse@v0.0.0-00010101000000-000000000000
go get distributedlogger.com/mongo@v0.0.0-00010101000000-000000000000
go build .
./main ./general_config.json
