#FROM ubuntu:24.04
#COPY . /home/distributed_logger
#RUN apt-get update -y && apt-get install -y python3 python3-pip python3-cxxheaderparser golang
#CMD ["/home/distributed_logger/examples/run_server.sh"]

# ---------- Stage 1: build ----------
FROM golang:1.22 AS builder

RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    python3.11-venv \
    cmake \
    && rm -rf /var/lib/apt/lists/*

# Create venv
RUN python3 -m venv /opt/venv

# Activate venv (via PATH)
ENV PATH="/opt/venv/bin:$PATH"

# Install Python deps inside venv
RUN pip install --no-cache-dir cxxheaderparser

WORKDIR /app

# Copy go.mod + local modules together
COPY server ./server
COPY generated ./generated
COPY tools ./tools
COPY examples ./examples

RUN ./tools/run_parser.sh ./examples/example_header.hh

WORKDIR /app/server/main

# Build binary
RUN go mod download
RUN go build -o distr_logger .

# ---------- Stage 2: runtime ----------
FROM ubuntu:22.04

WORKDIR /app

# Only runtime deps
RUN apt-get update && apt-get install -y ca-certificates && rm -rf /var/lib/apt/lists/*

# Copy binary only
COPY --from=builder /app/server/main/distr_logger ./server/main/distr_logger

# Copy config
COPY server/main/general_config.json ./server/main/general_config.json
COPY server/main/storage_config_mongo.json ./server/main/storage_config_mongo.json
COPY server/main/storage_config_clickhouse.json ./server/main/storage_config_clickhouse.json
COPY server/main/event_collector.json ./server/main/event_collector.json

COPY examples/run_server.sh ./examples/run_server.sh

#CMD ["main/distr_logger", "./general_config.json"]
CMD ["./examples/run_server.sh"]
#CMD ["ls"]
