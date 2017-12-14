FROM ubuntu AS flowgrindd

MAINTAINER Matt Mukerjee "mukerjee@cs.cmu.edu"

RUN apt-get update && apt-get install -y \
                              gcc \
                              cmake \
                              wget \
                              flowgrind \
    && rm -rf /var/lib/apt/lists/*

# Install pipework
WORKDIR /usr/local/bin
RUN wget https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework \
    && chmod +x pipework

# build SDRT adu-send and libVT
WORKDIR /root
RUN wget https://github.com/mukerjee/sdrt/archive/master.tar.gz \
    && tar xfz master.tar.gz \
    && cd sdrt-master/adu-send/lib \
    && make -j install \
    && cd /root \
    && wget https://github.com/mukerjee/libVT/archive/master.tar.gz \
    && tar xfz master.tar.gz.1 \
    && cd libVT-master \
    && make install \
    && cd /root \
    && rm -rf sdrt-master libVT-master master.tar.gz master.tar.gz.1

CMD pipework --wait \
    && pipework --wait -i eth2 \
    && LD_PRELOAD=libVT.so flowgrindd -d
