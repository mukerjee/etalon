FROM ubuntu:16.04 AS hadoop

MAINTAINER Matt Mukerjee "mukerjee@cs.cmu.edu"

WORKDIR /root
RUN apt-get update && apt-get install -y \
                              wget \
                              software-properties-common \
                              openssh-server \
                              openjdk-8-jdk \
                              maven \
                              wget \
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
    mv /tmp/config/id_rsa ~/.ssh/id_rsa && \
    mv /tmp/config/id_rsa.pub ~/.ssh/id_rsa.pub && \
    cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys && \
    mv /tmp/config/config ~/.ssh/config && \
    chmod 600 ~/.ssh/config && \
    chmod 400 ~/.ssh/id_rsa

RUN mkdir -p /usr/local/hadoop && \
    wget http://128.2.213.69/hadoop-3.0.0-SNAPSHOT.tar.gz && \
    tar xvzf hadoop-3.0.0-SNAPSHOT.tar.gz && \
    mv hadoop-3.0.0-SNAPSHOT/* /usr/local/hadoop/ && \
    mv /tmp/config/hadoop_config/* /usr/local/hadoop/etc/hadoop/ && \ 
    mkdir -p /usr/local/hadoop/hadoop_data/hdfs/namenode && \
    mkdir -p /usr/local/hadoop/hadoop_data/hdfs/datanode 

# Install pipework
WORKDIR /usr/local/bin
RUN wget https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework \
    && chmod +x pipework

# copy SDRT libADU and libVT
COPY libADU.so /usr/lib/libADU.so
COPY libVT.so /usr/lib/libVT.so

# hadoop
#RUN wget https://github.com/intel-hadoop/HiBench/archive/master.tar.gz \
#    && tar xfz master.tar.gz \
#    && mv HiBench-master HiBench \
#    && rm master.tar.gz

CMD pipework --wait \
    && pipework --wait -i eth2 \
    && sleep infinity 
