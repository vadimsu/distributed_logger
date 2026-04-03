#!/bin/bash
cd /home/distributed_logger
./tools/run_parser.sh ./examples/example_header.hh
cd examples/async_logging
rm -rf CMakeFiles CMakeCache.txt
cmake .
make
openssl req -x509 -newkey rsa:2048 -nodes -sha256 -days 365 \
  -keyout server.key -out server.crt \
  -subj "/C=CA/ST=BC/L=Vancouver/O=Distributed Logger/OU=IT/CN=localhost" \
  -addext "subjectAltName=DNS:localhost,IP:127.0.0.1"
#./bin/async_logging --certificate ./server.crt --key ./server.key --host 172.20.0.4 --port 7777
./bin/async_logging --host 172.20.0.4 --port 7777
/bin/bash
