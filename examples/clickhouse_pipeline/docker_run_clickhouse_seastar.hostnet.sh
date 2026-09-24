#!/bin/bash
sudo docker compose -f docker/config_clickhouse_seastar.hostnet.yaml up --build
