FROM ubuntu AS flowgrindd

MAINTAINER Matt Mukerjee "mukerjee@cs.cmu.edu"

RUN apt-get update && apt-get install -y \
                              wget \
                              flowgrind \
    && rm -rf /var/lib/apt/lists/*

# Install pipework
WORKDIR /usr/local/bin
RUN wget https://raw.githubusercontent.com/jpetazzo/pipework/master/pipework \
    && chmod +x pipework

# copy SDRT libADU and libVT
WORKDIR /usr/lib
COPY /usr/lib/libADU /usr/lib/
COPY /usr/lib/libVT /usr/lib/

CMD pipework --wait \
    && pipework --wait -i eth2 \
    && LD_PRELOAD=libVT.so flowgrindd -d
