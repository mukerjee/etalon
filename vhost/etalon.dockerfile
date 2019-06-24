FROM ubuntu:18.04

MAINTAINER Matt Mukerjee "mukerjee@cs.cmu.edu"

RUN apt-get update && apt-get install -y \
                              openssh-server \
                              openjdk-8-jdk \
			      python \
			      bc \
			      libcurl4-gnutls-dev \
			      libxmlrpc-core-c3-dev \
			      libpcap-dev \
			      libgsl-dev \
			      uuid-dev \
    && rm -rf /var/lib/apt/lists/*

# Install pipework
WORKDIR /usr/local/bin
ADD https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework /usr/local/bin/
RUN chmod +x pipework

# Install libVT
COPY libVT.so /usr/lib/libVT.so

# copy custom flowgrind
COPY flowgrindd /usr/local/sbin/flowgrindd

CMD pipework --wait && \
    pipework --wait -i eth2 && \
    sleep infinity
