FROM ubuntu:16.04

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

# ENV JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 \
#     HADOOP_INSTALL=/usr/local/hadoop \
#     PATH=$PATH:$HADOOP_INSTALL/bin \
#     PATH=$PATH:$HADOOP_INSTALL/sbin \
#     HADOOP_MAPRED_HOME=$HADOOP_INSTALL \
#     HADOOP_COMMON_HOME=$HADOOP_INSTALL \
#     HADOOP_HDFS_HOME=$HADOOP_INSTALL \
#     YARN_HOME=$HADOOP_INSTALL \
#     YARN_CONF_DIR=$YARN_HOME/etc/hadoop \
#     HADOOP_COMMON_LIB_NATIVE_DIR=$HADOOP_INSTALL/lib/native \
#     HADOOP_OPTS="-Djava.library.path=$HADOOP_INSTALL/lib/native" \
#     HADOOP_CONF_DIR=/usr/local/hadoop/etc/hadoop

# passwordless ssh setup
# WORKDIR /tmp/
# ADD config/ssh/* /root/.ssh/
# RUN chmod 600 /root/.ssh/id_rsa

# env vars for hadoop over SSH
# RUN echo PermitUserEnvironment yes >> /etc/ssh/sshd_config; \
#     echo JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64 >> /root/.ssh/environment; \
#     echo HADOOP_INSTALL=/usr/local/hadoop >> /root/.ssh/environment; \
#     echo PATH=$PATH:$HADOOP_INSTALL/bin >> /root/.ssh/environment; \
#     echo PATH=$PATH:$HADOOP_INSTALL/sbin >> /root/.ssh/environment; \
#     echo HADOOP_MAPRED_HOME=$HADOOP_INSTALL >> /root/.ssh/environment; \
#     echo HADOOP_COMMON_HOME=$HADOOP_INSTALL >> /root/.ssh/environment; \
#     echo HADOOP_HDFS_HOME=$HADOOP_INSTALL >> /root/.ssh/environment; \
#     echo YARN_HOME=$HADOOP_INSTALL >> /root/.ssh/environment; \
#     echo YARN_CONF_DIR=$YARN_HOME/etc/hadoop >> /root/.ssh/environment; \
#     echo HADOOP_COMMON_LIB_NATIVE_DIR=$HADOOP_INSTALL/lib/native >> /root/.ssh/environment; \
#     echo HADOOP_OPTS="-Djava.library.path=$HADOOP_INSTALL/lib/native" >> /root/.ssh/environment; \
#     echo HADOOP_CONF_DIR=/usr/local/hadoop/etc/hadoop >> /root/.ssh/environment

# install hadoop
# WORKDIR /usr/local/

# ENV hadoopver 2.9.0
# COPY hadoop-${hadoopver}.tar.gz .
# RUN tar xfvz hadoop-${hadoopver}.tar.gz && \
#     mv hadoop-${hadoopver} hadoop && \
#     rm hadoop-${hadoopver}.tar.gz && \
#     mkdir -p /usr/local/hadoop/hadoop_data/hdfs && \
#     mkdir -p /usr/local/hadoop/hadoop_data/hdfs-nn
# COPY config/hadoop_config/* ./hadoop/etc/hadoop/

# install HiBench
# WORKDIR /root/
# COPY HiBench.tar.gz /root/
# RUN tar xfvz HiBench.tar.gz && \
#     rm HiBench.tar.gz
# COPY config/hibench/*.conf /root/HiBench/conf/
# COPY config/hibench/dfsioe.conf /root/HiBench/conf/workloads/micro

# Install pipework
WORKDIR /usr/local/bin
ADD https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework /usr/local/bin/
RUN chmod +x pipework

# copy Etalon libADU and libVT
# COPY libADU.so /usr/lib/libADU.so
COPY libVT.so /usr/lib/libVT.so

# install fixed kill
COPY kill /bin/kill

# copy custom flowgrind
COPY flowgrindd /usr/local/sbin/flowgrindd

CMD pipework --wait \
    && pipework --wait -i eth2 \
    && sleep infinity
