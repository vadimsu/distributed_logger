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
RUN cd /distributed_logger/examples/seastar_app_logging && ./setup_seastar.sh
RUN cd /distributed_logger/examples/seastar_app_logging && rm -rf bin
RUN cd /distributed_logger/examples/seastar_app_logging && cmake . && make


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

#RUN bash ./install-dependencies.sh
WORKDIR /distributed_logger/examples/seastar_app_logging
CMD ["./bin/seastar_app_logging","--host","172.20.0.4", "--port", "7777", "--time","10","--overprovisioned", "-c", "5"]
