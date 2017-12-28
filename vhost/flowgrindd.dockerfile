FROM ubuntu AS flowgrindd

MAINTAINER Matt Mukerjee "mukerjee@cs.cmu.edu"

RUN apt-get update && apt-get install -y \
                              gcc \
                              cmake \
                              dh-autoreconf \
                              wget \
                              libcurl4-gnutls-dev \
                              libxmlrpc-core-c3-dev \
                              libpcap-dev \
                              libgsl-dev \
                              uuid-dev \
    && rm -rf /var/lib/apt/lists/*

# Install pipework
WORKDIR /usr/local/bin
RUN wget https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework \
    && chmod +x pipework

# copy SDRT libADU and libVT
COPY libADU.so /usr/lib/libADU.so
COPY libVT.so /usr/lib/libVT.so

# build custom flowgrind
WORKDIR /root
RUN wget https://github.com/mukerjee/flowgrind-sdrt/archive/master.tar.gz \
    && tar xfz master.tar.gz \
    && cd flowgrind-sdrt-master \
    && autoreconf -i \
    && ./configure \
    && make -j install \
    && cd /root \
    && rm -rf flowgrind-sdrt-master master.tar.gz

CMD pipework --wait \
    && pipework --wait -i eth2 \
    && LD_PRELOAD=libVT.so flowgrindd -d
