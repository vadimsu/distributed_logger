#!/bin/bash
#cd /app
cd seastar_based_server/bin
# Requires the matching `cpuset` pin in docker-compose (dedicated cores) -
# --overprovisioned disables busy-poll and --thread-affinity=0 disables core
# pinning, both of which defeat Seastar's per-core design and were the cause
# of the container throughput regression vs a native run.
#./seastar_server --config ./general_config.json -c 8 -m 8G --overprovisioned
#/distributed_logger/seastar_based_server/bin/seastar_server --help-seastar
#cd /tmp &&  /distributed_logger/seastar_based_server/bin/seastar_server --config ./general_config.json --overprovisioned --reactor-backend=io_uring --idle-poll-time-us 200 --poll-mode
./seastar_server --config ./general_config.json --overprovisioned --reactor-backend=io_uring --idle-poll-time-us 100 --poll-mode --cpuset 2-5
#./seastar_server 
