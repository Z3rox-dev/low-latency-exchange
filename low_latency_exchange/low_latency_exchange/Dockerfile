# Use the Ubuntu 23.10 image as the base
FROM ubuntu:23.10

# Environment variables to define versions
ARG CMAKE_VERSION=3.22.1
ARG PYTHON_VERSION=3.10.6
ARG BOOST_VERSION=1.74.0

# Update package manager, install dependencies and clean up in one layer to reduce image size
RUN apt-get update && apt-get install -y \
    build-essential \
    wget \
    git \
    gcc-11 \
    g++-11 \
    libssl-dev \
    libbenchmark-dev \
    libgtest-dev \
    zlib1g-dev \
    autoconf \
    automake \
    libtool \
    curl \
    make && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install Boost 1.74.0
RUN wget https://boostorg.jfrog.io/artifactory/main/release/1.74.0/source/boost_1_74_0.tar.gz && \
    tar -zxvf boost_1_74_0.tar.gz && \
    cd boost_1_74_0 && \
    ./bootstrap.sh && \
    ./b2 install && \
    cd .. && \
    rm -rf boost_1_74_0 boost_1_74_0.tar.gz

# Install CMake
RUN wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}.tar.gz && \
    tar -zxvf cmake-${CMAKE_VERSION}.tar.gz && \
    cd cmake-${CMAKE_VERSION} && \
    ./bootstrap && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    rm -rf cmake-${CMAKE_VERSION} cmake-${CMAKE_VERSION}.tar.gz

# Install Python 3.10.6
RUN wget https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz && \
    tar -zxvf Python-${PYTHON_VERSION}.tgz && \
    cd Python-${PYTHON_VERSION} && \
    ./configure --enable-optimizations && \
    make -j$(nproc) && \
    make altinstall && \
    cd .. && \
    rm -rf Python-${PYTHON_VERSION} Python-${PYTHON_VERSION}.tgz

# Install pip for Python 3.10.6
RUN wget https://bootstrap.pypa.io/get-pip.py && \
    python3.10 get-pip.py && \
    rm get-pip.py

# Set the working directory
WORKDIR /workspace/low_latency_engine

# Copy the current directory contents into the container
COPY . /workspace/low_latency_engine/

# Create the build directory and set it as the working directory
RUN mkdir -p TradingEngine/build
WORKDIR /workspace/low_latency_engine/TradingEngine/build

# Configure and build the project with CMake
RUN cmake .. && \
    cmake --build .