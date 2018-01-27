FROM ubuntu:16.04 AS hadoop

MAINTAINER Matt Mukerjee "mukerjee@cs.cmu.edu"

WORKDIR /root
RUN apt-get update && apt-get install -y \
                              openssh-server \
                              openjdk-8-jdk \
                              maven \
                              iputils-ping \
                              python \
                              bc \
    && rm -rf /var/lib/apt/lists/*

ENV JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
ENV HADOOP_INSTALL=/usr/local/hadoop
ENV PATH=$PATH:$HADOOP_INSTALL/bin
ENV PATH=$PATH:$HADOOP_INSTALL/sbin
ENV HADOOP_MAPRED_HOME=$HADOOP_INSTALL
ENV HADOOP_COMMON_HOME=$HADOOP_INSTALL
ENV HADOOP_HDFS_HOME=$HADOOP_INSTALL
ENV YARN_HOME=$HADOOP_INSTALL
ENV HADOOP_COMMON_LIB_NATIVE_DIR=$HADOOP_INSTALL/lib/native
ENV HADOOP_OPTS="-Djava.library.path=$HADOOP_INSTALL/lib"
ENV HADOOP_CONF_DIR=/usr/local/hadoop/etc/hadoop

COPY config /tmp/config

# passwordless ssh setup
RUN mkdir -p /root/.ssh && \
    mv /tmp/config/ssh/* ~/.ssh/ && \
    cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys && \
    chmod 600 ~/.ssh/config && \
    chmod 400 ~/.ssh/id_rsa

WORKDIR /usr/local/
COPY hadoop-2.7.5.tar.gz .
RUN tar xfvz hadoop-2.7.5.tar.gz && \
    mv hadoop-2.7.5 hadoop && \
    rm hadoop-2.7.5.tar.gz && \
    mv /tmp/config/hadoop_config/* /usr/local/hadoop/etc/hadoop/ && \
    mkdir -p /usr/local/hadoop/hadoop_data/hdfs/namenode && \
    mkdir -p /usr/local/hadoop/hadoop_data/hdfs/datanode

# COPY HiBench.tar.gz /root/
# RUN tar xfvz HiBench.tar.gz

# Install pipework
WORKDIR /usr/local/bin
ADD https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework /usr/local/bin/
RUN chmod +x pipework

# copy SDRT libADU and libVT
COPY libADU.so /usr/lib/libADU.so
COPY libVT.so /usr/lib/libVT.so

# build fixed kill
# WORKDIR /root/
# RUN apt-get update
# RUN apt-get install -y git
# RUN apt-get install -y autoconf
# RUN apt-get install -y automake
# RUN apt-get install -y libtool
# RUN apt-get install -y libtool-bin
# RUN apt-get install -y autopoint
# RUN apt-get install -y pkg-config
# RUN apt-get install -y libncurses5-dev
# RUN apt-get install -y gettext
#     # && rm -rf /var/lib/apt/lists/*
# RUN git clone https://gitlab.com/procps-ng/procps.git && \
#     cd procps && \
#     ./autogen.sh && \
#     ./configure && \
#     make && \
#     make install

COPY kill /bin/kill

CMD pipework --wait \
    && pipework --wait -i eth2 \
    && sleep infinity 
