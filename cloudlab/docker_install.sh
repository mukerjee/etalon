FROM ubuntu

MAINTAINER Matt Mukerjee "mukerjee@cs.cmu.edu"

RUN apt-get update && apt-get install -y \
                              openjdk-7-jdk \
                              maven \
                              iperf \
                              iperf3 \
                              net-tools \
                              iputils-ping \
    && rm -rf /var/lib/apt/lists/*

ADD https://github.com/intel-hadoop/HiBench/archive/master.tar.gz $HOME/HiBench.tar.gz
RUN tar xfz $HOME/HiBench.tar.gz $HOME/

ADD https://github.com/flowgrind/flowgrind/archive/master.tar.gz $HOME/flowgrind.tar.gz
RUN tar xfz $HOME/flowgrind.tar.gz $HOME/
RUN cd $HOME/flowgrind
RUN autoreconf -i
RUN ./configure
RUN make -j

ADD https://github.com/mukerjee/sdrt/archive/master.tar.gz $HOME/sdrt.tar.gz
RUN tar xfz $HOME/sdrt/tar.gz $HOME/
RUN cd $HOME/sdrt/send-adu/lib
RUN make install -j

WORKDIR $HOME
ENTRYPOINT ["flowgrind"]
CMD ["--help"]
