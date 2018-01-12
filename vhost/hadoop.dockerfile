FROM ubuntu:14.04 AS hadoop

MAINTAINER Matt Mukerjee "mukerjee@cs.cmu.edu"

RUN apt-get update && apt-get install -y \
                              wget \
                              software-properties-common \
                              openssh-server \
    && rm -rf /var/lib/apt/lists/*

#RUN mkdir /var/run/sshd
#RUN chmod 0755 /var/run/sshd
#RUN echo 'root:root' |chpasswd
#RUN sed -ri 's/^PermitRootLogin\s+.*/PermitRootLogin yes/' /etc/ssh/sshd_config
#RUN sed -ri 's/UsePAM yes/#UsePAM yes/g' /etc/ssh/sshd_config
RUN ssh-keygen -t rsa -f ~/.ssh/id_rsa -P '' && \
    cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys
#EXPOSE 22

#RUN add-apt-repository ppa:openjdk-r/ppa 
RUN apt-get update && apt-get install -y \
                              openjdk-8-jdk \
                              maven \
                              wget \
    && rm -rf /var/lib/apt/lists/*

# Install pipework
WORKDIR /usr/local/bin
RUN wget https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework \
    && chmod +x pipework

# copy SDRT libADU and libVT
COPY libADU.so /usr/lib/libADU.so
COPY libVT.so /usr/lib/libVT.so

# hadoop
WORKDIR /root
#RUN wget https://github.com/intel-hadoop/HiBench/archive/master.tar.gz \
#    && tar xfz master.tar.gz \
#    && mv HiBench-master HiBench \
#    && rm master.tar.gz

CMD service ssh start 
CMD pipework --wait \
    && pipework --wait -i eth2 \
    && sleep infinity 
