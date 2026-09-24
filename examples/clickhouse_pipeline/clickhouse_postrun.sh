#!/bin/bash
echo "=== Clickhouse Verification ==="

echo "Total records:"
sudo docker exec docker-clickhouse-1 clickhouse-client -u default --password default --query "SELECT COUNT(1) FROM events WHERE 1"

echo ""
echo "Sample records:"

echo "RAW events table"
sudo docker exec docker-clickhouse-1 clickhouse-client -u default --password default --query "DESCRIBE events"
sudo docker exec docker-clickhouse-1 clickhouse-client -u default --password default --query "SELECT * FROM events WHERE 1 LIMIT 5"

echo "events_event0"
sudo docker exec docker-clickhouse-1 clickhouse-client -u default --password default --query "DESCRIBE events_event0"
sudo docker exec docker-clickhouse-1 clickhouse-client -u default --password default --query "SELECT * FROM events_event0 WHERE 1 LIMIT 5"

echo "events_event1"
sudo docker exec docker-clickhouse-1 clickhouse-client -u default --password default --query "DESCRIBE events_event1"
sudo docker exec docker-clickhouse-1 clickhouse-client -u default --password default --query "SELECT * FROM events_event1 WHERE 1 ORDER BY Timestamp DESC LIMIT 5"

echo "mv_events_event0"
sudo docker exec docker-clickhouse-1 clickhouse-client -u default --password default --query "DESCRIBE mv_events_event0"
sudo docker exec docker-clickhouse-1 clickhouse-client -u default --password default --query "SELECT * FROM mv_events_event0 WHERE 1 LIMIT 5"

echo "mv_events_event1"
sudo docker exec docker-clickhouse-1 clickhouse-client -u default --password default --query "DESCRIBE mv_events_event1"
sudo docker exec docker-clickhouse-1 clickhouse-client -u default --password default --query "SELECT * FROM mv_events_event1 WHERE 1 ORDER BY Timestamp DESC LIMIT 5"
