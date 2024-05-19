# Copyright (C) 2020  Christian Berger
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# First stage for building the software:
FROM ubuntu:18.04 as builder
MAINTAINER Christian Berger "christian.berger@gu.se"

ENV DEBIAN_FRONTEND noninteractive

# Upgrade the Ubuntu 18.04 LTS base image
RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get dist-upgrade -y

# Install the development libraries for OpenCV
RUN apt-get install -y --no-install-recommends \
    ca-certificates \
    cmake \
    build-essential \
    libopencv-dev

# Include this source tree and compile the sources
ADD . /opt/sources
WORKDIR /opt/sources
RUN mkdir build && \
    cd build && \
    cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/tmp .. && \
    make && make install


# Second stage for packaging the software into a software bundle:
# Start from Ubuntu 18.04
FROM ubuntu:18.04
MAINTAINER Christian Berger "christian.berger@gu.se"

ENV DEBIAN_FRONTEND=noninteractive


RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get dist-upgrade -y

RUN apt-get install -y --no-install-recommends \
    libopencv-core3.2 \
    libopencv-highgui3.2 \
    libopencv-imgproc3.2 

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    software-properties-common \
    build-essential \
    python3 \
    python3-pip \
    python3-dev \
    python3-numpy \
    python3-opencv \
    python3-protobuf \
    protobuf-compiler \
    git \
    make \
    wget && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Add the libcluon PPA and install libcluon
RUN add-apt-repository ppa:chrberger/libcluon && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    libcluon && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*


# install packages
RUN pip3 install --no-cache-dir joblib pandas scikit-learn

# copy the compiled sources from the first stage
WORKDIR /usr/bin
COPY --from=builder /tmp/bin/main .

# Copy LRegressionModel directory from CURRENT directory to the container
COPY tempML /usr/bin/tempML

WORKDIR /usr/bin/tempML

RUN make

WORKDIR /usr/bin

# create a bash script to run the python software in LRegressionModel Directory in the background (needs to be started first), then run the compiled C++ software
# Create a bash script to run the python software in the background, then run the compiled C++ software
# remember that the compiled c++ software needs to take argguments from docke rrun command
RUN echo "#!/bin/bash" > run.sh && \
    echo "cd /usr/bin/tempML && python3 receiveEnvelopes.py > /dev/null 2>&1 &" >> run.sh && \
    # echo "sleep 5" >> run.sh && \
    echo "cd /usr/bin && ./main \$@" >> run.sh && \
    chmod +x run.sh

# Set the working directory in the container
RUN ls -la

# Set the working directory in the container


# # Run the bash script
ENTRYPOINT ["/usr/bin/run.sh"]
CMD ["--cid=253", "--name=img", "--width=640", "--height=480", "--verbose"]
