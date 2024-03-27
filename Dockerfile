# Section 1: Build the application
FROM ubuntu:22.04 as builder

RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get dist-upgrade -y

RUN apt-get install -y --no-install-recommends \
    cmake \
    build-essential

ADD . /opt/sources
WORKDIR /opt/sources

RUN cd /opt/sources && \
    mkdir build && \
    cd build && \
    cmake -D CMAKE_BUILD_TYPE=Release .. && \
    make

##################################################
# Section 2: Bundle the application.
FROM ubuntu:22.04

RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get dist-upgrade -y

WORKDIR /opt
COPY --from=builder /opt/sources/build/helloworld /opt/helloworld
ENTRYPOINT ["/opt/helloworld"]