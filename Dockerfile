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
    python3.7 \
    python3.7-dev \
    python3.7-distutils \
    python3-opencv \
    python3-protobuf \
    protobuf-compiler \
    git \
    make \
    wget \
    libopenblas-dev \
    liblapack-dev \
    gfortran && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Fix for 'apt_pkg' module issue
RUN ln -s /usr/lib/python3/dist-packages/apt_pkg.cpython-36m-aarch64-linux-gnu.so /usr/lib/python3/dist-packages/apt_pkg.so || true

# Use Python 3.7 as the default python3
RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.7 1

# Install pip for Python 3.7
RUN wget https://bootstrap.pypa.io/get-pip.py && python3.7 get-pip.py && rm get-pip.py

RUN python3 -m pip install --upgrade pip setuptools wheel

RUN pip3 install --no-cache-dir joblib==1.1.1

# Install specific versions of numpy and scipy from pre-built binaries for compatibility with ARM
RUN pip3 install --no-cache-dir numpy==1.19.5 scipy==1.5.4

# Install scikit-learn with the prefer-binary flag and specific version
RUN pip3 install --no-cache-dir --prefer-binary scikit-learn==0.24.2

# Install pandas with the prefer-binary flag
RUN pip3 install --no-cache-dir --prefer-binary pandas==1.1.5

# Copy the compiled sources from the first stage
WORKDIR /usr/bin
COPY --from=builder /tmp/bin/main .

# Copy LRegressionModel directory from CURRENT directory to the container
COPY tempML /usr/bin/tempML

WORKDIR /usr/bin/tempML

RUN make

WORKDIR /usr/bin

# Create a bash script to run the python software in the background, then run the compiled C++ software
RUN echo "#!/bin/bash" > run.sh && \
    echo "cd /usr/bin/tempML && python3 receiveEnvelopes.py > /dev/null 2>&1 &" >> run.sh && \
    echo "cd /usr/bin && ./main \$@" >> run.sh && \
    chmod +x run.sh

# Set the working directory in the container
RUN ls -la

# Run the bash script
ENTRYPOINT ["/usr/bin/run.sh"]
CMD ["--cid=253", "--name=img", "--width=640", "--height=480", "--verbose"]
