FROM ubuntu:16.04

MAINTAINER Matt Mukerjee "mukerjee@cs.cmu.edu"

RUN apt-get update && apt-get install -y \
                              openssh-server \
                              openjdk-8-jdk \
    && rm -rf /var/lib/apt/lists/*

ENV JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64
ENV HADOOP_INSTALL=/usr/local/hadoop
ENV PATH=$PATH:$HADOOP_INSTALL/bin
ENV PATH=$PATH:$HADOOP_INSTALL/sbin
ENV HADOOP_MAPRED_HOME=$HADOOP_INSTALL
ENV HADOOP_COMMON_HOME=$HADOOP_INSTALL
ENV HADOOP_HDFS_HOME=$HADOOP_INSTALL
ENV YARN_HOME=$HADOOP_INSTALL
ENV YARN_CONF_DIR=$YARN_HOME/etc/hadoop
ENV HADOOP_COMMON_LIB_NATIVE_DIR=$HADOOP_INSTALL/lib/native
ENV HADOOP_OPTS="-Djava.library.path=$HADOOP_INSTALL/lib/native"
ENV HADOOP_CONF_DIR=/usr/local/hadoop/etc/hadoop

# passwordless ssh setup
WORKDIR /tmp/
ADD config/ssh ~/.ssh

# install hadoop
WORKDIR /usr/local/
COPY hadoop-2.9.0.tar.gz .
RUN tar xfvz hadoop-2.9.0.tar.gz && \
    mv hadoop-2.9.0 hadoop && \
    rm hadoop-2.9.0.tar.gz && \
    mkdir -p /usr/local/hadoop/hadoop_data/hdfs && \
    mkdir -p /usr/local/hadoop/hadoop_data/hdfs-nn

# install HiBench
WORKDIR /root/
COPY HiBench.tar.gz /root/
RUN tar xfvz HiBench.tar.gz && \
    rm HiBench.tar.gz

# Install pipework
WORKDIR /usr/local/bin
ADD https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework /usr/local/bin/
RUN chmod +x pipework

# copy Etalon libADU and libVT
COPY libADU.so /usr/lib/libADU.so
COPY libVT.so /usr/lib/libVT.so

# install fixed kill
COPY kill /bin/kill

# copy custom flowgrind
COPY flowgrindd /usr/local/sbin/flowgrindd

CMD pipework --wait \
    && pipework --wait -i eth2 \
    && sleep infinity 
