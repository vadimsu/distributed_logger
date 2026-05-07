#!/bin/bash
echo "=== Mongo Verification ==="

echo "Total records:"
sudo docker exec docker-mongo-1 mongosh eventdemo --quiet --eval \
'db.events.countDocuments()'

echo ""
echo "Sample records:"
sudo docker exec docker-mongo-1 mongosh eventdemo --quiet --eval \
'db.events.find().limit(3).pretty()'
