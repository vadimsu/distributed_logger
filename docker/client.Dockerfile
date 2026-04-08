FROM gcc:13
#FROM ubuntu:24.04
COPY . /home/distributed_logger

#RUN apt-get update -y && apt-get install -y python3 python3-pip python3-cxxheaderparser cmake openssl libssl-dev libboost-all-dev
RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 \
    python3-pip \
    python3-venv \
    cmake \
    openssl \
    libssl-dev \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

# Create venv
RUN python3 -m venv /opt/venv

# Activate venv (via PATH)
ENV PATH="/opt/venv/bin:$PATH"

# Install Python deps inside venv
RUN pip install --no-cache-dir cxxheaderparser

CMD ["/home/distributed_logger/examples/run_client.sh"]
