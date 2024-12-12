# Start with Ubuntu as base image
FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install essential packages
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    curl \
    git \
    ninja-build \
    pkg-config \
    tar \
    unzip \
    zip \
    libpq-dev \  
    autoconf \ 
    automake \ 
    libtool \ 
    && rm -rf /var/lib/apt/lists/*

# Set up working directory
WORKDIR /app

# Install vcpkg
RUN git clone https://github.com/Microsoft/vcpkg.git /opt/vcpkg && \
    /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics

# Add vcpkg to PATH
ENV PATH="/opt/vcpkg:${PATH}"

# Set vcpkg root
ENV VCPKG_ROOT=/opt/vcpkg

# Copy your project files
COPY . .

# Create build directory and run cmake
RUN mkdir -p build && cd build
RUN cd build && cmake --preset=vcpkg ..
RUN cd build && cmake --build .

# Set the default command
CMD ["./build/MercuryLockbox"]