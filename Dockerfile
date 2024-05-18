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


# Update and install necessary packages
RUN apt-get update && \
    apt-get install -y software-properties-common wget curl ca-certificates && \
    add-apt-repository ppa:deadsnakes/ppa && \
    apt-get update && \
    apt-get install -y python3.8 python3.8-distutils python3.8-dev && \
    update-ca-certificates

# Install pip using curl and Python 3.8
RUN curl -sS https://bootstrap.pypa.io/get-pip.py | python3.8

# Set Python 3.8 as the default Python version
RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.8 1

# Check Python and Pip versions
RUN python3 -m pip --version && python3 --version

# install packages
RUN pip install --no-cache-dir joblib pycluon pandas scikit-learn

# copy the compiled sources from the first stage
WORKDIR /usr/bin
COPY --from=builder /tmp/bin/main .

# Copy LRegressionModel directory from CURRENT directory to the container
COPY LRegressionModel /usr/bin/LRegressionModel


# create a bash script to run the python software in LRegressionModel Directory in the background (needs to be started first), then run the compiled C++ software
# Create a bash script to run the python software in the background, then run the compiled C++ software
# remember that the compiled c++ software needs to take argguments from docke rrun command
RUN echo "#!/bin/bash" > run.sh && \
    echo "cd /usr/bin/LRegressionModel && python3 cluontest.py > /dev/null 2>&1 &" >> run.sh && \
    echo "sleep 5" >> run.sh && \
    echo "cd /usr/bin && ./main \$@" >> run.sh && \
    chmod +x run.sh

# Set the working directory in the container
RUN ls -la

# Set the working directory in the container


# # Run the bash script
ENTRYPOINT ["/usr/bin/run.sh"]
CMD ["--cid=253", "--name=img", "--width=640", "--height=480", "--verbose"]
