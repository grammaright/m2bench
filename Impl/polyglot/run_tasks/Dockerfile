FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC 

# Directory 
RUN mkdir -p /data

##############################
# Install Dependencies
#############################

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    time \
    gcc-10 \
    g++-10 \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    ninja-build \
    graphviz \
    graphviz-dev

# Install OpenBLAS
RUN cd /data && \
    git clone --depth 1 -b v0.3.0 https://github.com/OpenMathLib/OpenBLAS.git && \
    cd OpenBLAS && \
    make -j9 && \
    make install

# Install LAPACK
RUN apt-get install -y liblapacke-dev liblapack-dev

##############################
# Setup
#############################
# Copy source code
COPY ./docker/build /data

# Install vcpkg
RUN cd /data && \
    git clone https://github.com/microsoft/vcpkg.git && \
    cd vcpkg && \
    ./bootstrap-vcpkg.sh
ENV VCPKG_ROOT=/data/vcpkg
ENV VCPKG_TOOLCHAIN_PATH=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
ENV PATH=$VCPKG_ROOT:$PATH

# Compile PreVision
ENV LD_LIBRARY_PATH=/opt/OpenBLAS/lib
RUN ls -lha /data/
RUN cd /data/prevision/ && \
    cmake --preset=default && \
    cmake --build build
ENV PREVISION_PATH=/data/prevision/

# Compile duckdb-spatial and duckdb
RUN cd /data/duckdb-spatial && GEN=ninja make release

RUN cd /data/duckdb && GEN=ninja make release
ENV DUCKDB_PATH=/data/duckdb/

# Compile m2
RUN cd /data/m2bench/Impl/polyglot/run_tasks/thirdparty/velocypack && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j9
RUN cd /data/m2bench/Impl/polyglot/run_tasks/thirdparty/libspatialindex && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j9
RUN cd /data/m2bench/Impl/polyglot/run_tasks && \
    cmake --preset=default && \
    cmake --build build
ENV M2_PATH=/data/m2bench/Impl/run_tasks/

# Install spatial extension
RUN mkdir -p /root/.duckdb/extensions/$(cd /data/duckdb-spatial/duckdb/; git rev-parse --short HEAD)/linux_amd64/
RUN cp /data/duckdb-spatial/build/release/extension/spatial/spatial.duckdb_extension \
    /root/.duckdb/extensions/$(cd /data/duckdb-spatial/duckdb/; git rev-parse --short HEAD)/linux_amd64/spatial.duckdb_extension 