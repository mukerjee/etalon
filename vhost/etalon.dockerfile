FROM ubuntu:18.04

MAINTAINER Matt Mukerjee "mukerjee@cs.cmu.edu"

# Install dependencies.
RUN apt-get update && apt-get install -y \
        bc inetutils-ping libcurl4-gnutls-dev libgsl-dev libpcap-dev \
        libxmlrpc-core-c3-dev net-tools openssh-server openjdk-8-jdk python \
        tcpdump uuid-dev && \
    rm -rf /var/lib/apt/lists/*

# Download pipework.
WORKDIR /usr/local/bin
ADD https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework /usr/local/bin/
RUN chmod +x pipework

# Copy libVT.
COPY libVT.so /usr/lib/libVT.so

# Copy custom flowgrind.
COPY flowgrindd /usr/local/sbin/flowgrindd

CMD pipework --wait -i eth1 && \
    pipework --wait -i eth2 && \
    sleep infinity
