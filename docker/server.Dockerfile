FROM ubuntu:24.04
COPY . /home/distributed_logger
CMD ["/home/distributed_logger/examples/run_server.sh"]
RUN apt-get update -y && apt-get install -y python3 python3-pip python3-cxxheaderparser ca-certificates golang && update-ca-certificates
RUN openssl req -x509 -newkey rsa:2048 -keyout /home/distributed_logger/server/main/key.pem -out /home/distributed_logger/server/main/cert.pem -sha256 -days 365 -nodes \
  -subj "/CN=localhost" \
  -addext "subjectAltName = DNS:localhost, IP:127.0.0.1, IP:172.20.0.4"

