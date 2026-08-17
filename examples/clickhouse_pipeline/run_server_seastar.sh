#!/bin/bash
#cd /app
cd seastar_based_server/bin
./seastar_server --config ./general_config.json --thread-affinity=0 --overprovisioned
#./seastar_server 
