#FROM ubuntu:plucky  as seastar_builder
FROM ubuntu:24.04

ENV TZ=Etc/UTC
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN apt-get update && apt-get install -y tzdata git python3 python3-pip python3-venv

# Create venv
RUN python3 -m venv /opt/venv

# Activate venv (via PATH)
ENV PATH="/opt/venv/bin:$PATH"

# Install Python deps inside venv
RUN pip install --no-cache-dir cxxheaderparser

#COPY ./examples/seastar_app_logging/setup_seastar.sh ./setup_seastar.sh
COPY . distributed_logger

RUN cd /distributed_logger && ./tools/run_parser.sh ./examples/example_header.hh
RUN apt-get update
RUN apt install -y libssl-dev
RUN cd /distributed_logger/seastar_based_server && ./setup_seastar.sh
RUN cd /distributed_logger/seastar_based_server && rm -rf bin
RUN cd /distributed_logger/seastar_based_server && rm -rf CMakeCache.txt
RUN cd /distributed_logger/seastar_based_server && rm -rf CMakeFiles
RUN cd /distributed_logger/seastar_based_server && cmake . && make


#FROM debian:bookworm-slim
#FROM ubuntu:24.04
RUN apt-get update && apt-get install -y gpg
#RUN echo "deb [trusted=yes] http://deb.debian.org/debian trixie main" >> /etc/apt/sources.list
#RUN sleep 3
# Official Ubuntu 24.04 Repositories
RUN echo "deb http://ubuntu.com noble main restricted universe multiverse" > /etc/apt/sources.list.d/noble.list && \
    echo "deb http://ubuntu.com noble-updates main restricted universe multiverse" >> /etc/apt/sources.list.d/noble.list && \
    echo "deb http://ubuntu.com noble-security main restricted universe multiverse" >> /etc/apt/sources.list.d/noble.list

#RUN apt-get update
#RUN gpg --no-default-keyring --keyring /usr/share/keyrings/debian-trixie.gpg --keyserver keyserver.ubuntu.com --recv-keys
#RUN apt-get update

#COPY --from=seastar_builder /distributed_logger/examples/seastar_app_logging/bin/seastar_app_logging ./seastar_app_logging
#COPY --from=seastar_builder /distributed_logger/examples/seastar_app_logging/seastar/install-dependencies.sh ./install-dependencies.sh
RUN apt-get install -y clickhouse-client
#RUN bash ./install-dependencies.sh
WORKDIR /distributed_logger

# Copy config
COPY examples/clickhouse_pipeline/general_config_seastar.json seastar_based_server/bin/general_config.json
COPY examples/clickhouse_pipeline/storage_config_clickhouse_seastar.json seastar_based_server/bin/storage_config_clickhouse.json
COPY examples/clickhouse_pipeline/event_collector_seastar.json seastar_based_server/bin/event_collector.json

COPY examples/clickhouse_pipeline/run_server_seastar.sh seastar_based_server/bin/run_server_seastar.sh

CMD ["seastar_based_server/bin/run_server_seastar.sh"]
