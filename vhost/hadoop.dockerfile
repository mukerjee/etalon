FROM ubuntu AS hadoop

MAINTAINER Matt Mukerjee "mukerjee@cs.cmu.edu"

RUN apt-get update && apt-get install -y \
                              gcc \
                              cmake \
                              wget \
                              software-properties-common \
    && rm -rf /var/lib/apt/lists/*

RUN add-apt-repository ppa:openjdk-r/ppa 
RUN apt-get update && apt-get install -y \
                              openjdk-7-jdk \
                              maven \
                              wget \
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

# hadoop
WORKDIR /root
RUN wget https://github.com/intel-hadoop/HiBench/archive/master.tar.gz \
    && tar xfz master.tar.gz \
    && mv HiBench-master HiBench \
    && rm master.tar.gz

CMD pipework --wait \
    && pipework --wait -i eth2
