FROM gcc:13 as builder

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt/lists,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
    python3 \
    python3-pip \
    python3-venv \
    cmake \
    openssl \
    libssl-dev \
    libboost-program-options-dev \
    && rm -rf /var/lib/apt/lists/*

# Create venv
RUN python3 -m venv /opt/venv

# Activate venv (via PATH)
ENV PATH="/opt/venv/bin:$PATH"

# Install Python deps inside venv
RUN pip install --no-cache-dir cxxheaderparser

COPY . /home/distributed_logger

WORKDIR /home/distributed_logger

RUN ./tools/run_parser.sh ./examples/example_header.hh

COPY ./examples/async_logging/CMakeLists.txt ./examples/async_logging/

WORKDIR ./examples/async_logging

RUN rm CMakeCache.txt && cmake . && make

RUN strings /usr/lib/x86_64-linux-gnu/libstdc++.so.6 | grep GLIBCXX

FROM debian:bookworm-slim

RUN echo "deb http://deb.debian.org/debian trixie main" >> /etc/apt/sources.list

RUN apt-get update && apt-get install -y --no-install-recommends -t \
    trixie \
    libstdc++6

RUN apt-get install -y --no-install-recommends \
    libssl3 \
    libboost-program-options1.74-dev \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /home/distributed_logger/examples/async_logging/bin/async_logging ./async_logging
CMD ["./async_logging","--host","172.20.0.4", "--port", "7777"]
