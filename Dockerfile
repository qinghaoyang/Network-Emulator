# Very simple dockerfile

# Getting latest ubuntu version
FROM ubuntu:20.04 AS focal

# updating and installing dependencies for MP1 and MP2
RUN apt update && \
    apt-get install -y build-essential python python3 zip net-tools iptables sudo curl
