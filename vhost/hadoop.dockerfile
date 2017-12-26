FROM ubuntu AS hadoop

MAINTAINER Matt Mukerjee "mukerjee@cs.cmu.edu"

RUN apt-get update && apt-get install -y \
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

# copy SDRT libADU and libVT
WORKDIR /usr/lib
COPY /usr/lib/libADU /usr/lib/
COPY /usr/lib/libVT /usr/lib/

# hadoop
WORKDIR /root
RUN wget https://github.com/intel-hadoop/HiBench/archive/master.tar.gz \
    && tar xfz master.tar.gz \
    && mv HiBench-master HiBench \
    && rm master.tar.gz

CMD pipework --wait \
    && pipework --wait -i eth2
