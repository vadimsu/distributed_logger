FROM ubuntu:24.04
COPY . /home/distributed_logger
RUN apt-get update -y && apt-get install -y python3 python3-pip python3-cxxheaderparser golang
CMD ["/home/distributed_logger/examples/run_server.sh"]
