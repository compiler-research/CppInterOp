# Dockerfile for building and running CppInterOp inside a container.
# This image includes EMSDK for WebAssembly builds and optional CUDA support.
# Usage:
#   Basic:     docker build -t cppinterop:dev .
#   With CUDA: docker build -t cppinterop:cuda --build-arg USE_CUDA=1 .

# Start with Ubuntu base
FROM ubuntu:24.04

# Build argument for CUDA support
ARG USE_CUDA=0

# If USE_CUDA=1, switch to CUDA base
FROM nvidia/cuda:12.3.1-devel-ubuntu24.04 AS build_image_cuda
FROM ubuntu:24.04 AS build_image_nocuda
FROM build_image_${USE_CUDA:+cuda}${USE_CUDA:-nocuda}

ARG DEBIAN_FRONTEND=noninteractive
ARG EMSDK_VERSION=3.1.45

# Install system dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    git \
    python3 \
    python3-venv \
    python3-pip \
    ninja-build \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Pre-install EMSDK (optional, the build script can bootstrap if needed)
RUN git clone https://github.com/emscripten-core/emsdk.git /opt/emsdk && \
    cd /opt/emsdk && \
    ./emsdk install ${EMSDK_VERSION} && \
    ./emsdk activate ${EMSDK_VERSION} && \
    echo "source /opt/emsdk/emsdk_env.sh" >> /etc/bash.bashrc

# Create a user to avoid running as root in the container
RUN useradd -m builder

WORKDIR /home/builder

# Copy the repo (when building, use build context root) and script
COPY . /home/builder/src
WORKDIR /home/builder/src

# Make the build helper executable
RUN chmod +x /home/builder/src/scripts/container_build.py

# Expose a port that downstream web demos might use. Adjust as needed.
EXPOSE 8080

# Default is to build for linux. Users can override CMD args for platform, build-dir, etc.
ENTRYPOINT ["/usr/bin/env", "python3", "/home/builder/src/scripts/container_build.py"]
