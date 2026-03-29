#!/bin/bash
curl https://clickhouse.com/ | sh
./clickhouse install -y
/usr/bin/clickhouse-server --config-file /home/distributed_logger/examples/config.xml &
echo $(pwd)
cd /home/distributed_logger
./tools/run_parser.sh ./examples/example_header.hh
cd server/main
go build .
#echo "launching clickhouse"
#clickhouse-server &
echo "clickhouse is running "
ps -A|grep clickhouse
echo "launching clickhouse client"
clickhouse-client
/bin/bash
#./main general_config.json
#while true;do
#	echo 'waiting'
#	sleep 1
#done
