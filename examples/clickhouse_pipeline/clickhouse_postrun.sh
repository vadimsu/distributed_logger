#!/bin/bash
echo "=== Clickhouse Verification ==="

echo "Total records:"
sudo docker exec docker-clickhouse-1 clickhouse-client --query "SELECT COUNT(1) FROM events WHERE 1"

echo ""
echo "Sample records:"

echo "RAW events table"
sudo docker exec docker-clickhouse-1 clickhouse-client --query "DESCRIBE events"
sudo docker exec docker-clickhouse-1 clickhouse-client --query "SELECT * FROM events WHERE 1 LIMIT 5"

echo "events_event0"
sudo docker exec docker-clickhouse-1 clickhouse-client --query "DESCRIBE events_event0"
sudo docker exec docker-clickhouse-1 clickhouse-client --query "SELECT * FROM events_event0 WHERE 1 LIMIT 5"

echo "events_event1"
sudo docker exec docker-clickhouse-1 clickhouse-client --query "DESCRIBE events_event1"
sudo docker exec docker-clickhouse-1 clickhouse-client --query "SELECT * FROM events_event1 WHERE 1 LIMIT 5"

echo "mv_events_event0"
sudo docker exec docker-clickhouse-1 clickhouse-client --query "DESCRIBE mv_events_event0"
sudo docker exec docker-clickhouse-1 clickhouse-client --query "SELECT * FROM mv_events_event0 WHERE 1 LIMIT 5"

echo "mv_events_event1"
sudo docker exec docker-clickhouse-1 clickhouse-client --query "DESCRIBE mv_events_event1"
sudo docker exec docker-clickhouse-1 clickhouse-client --query "SELECT * FROM mv_events_event1 WHERE 1 LIMIT 5"
