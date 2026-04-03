FROM ubuntu:24.04
COPY . /home/distributed_logger
CMD ["/home/distributed_logger/examples/run_client.sh"]
RUN apt-get update -y && apt-get install -y python3 python3-pip python3-cxxheaderparser cmake openssl libssl-dev libboost-all-dev
