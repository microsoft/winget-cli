FROM ubuntu:19.04

RUN apt-get update \
    && apt-get install -y \
        apt-utils \
    && apt-get install -y --no-install-recommends \
        apt-transport-https \
        ca-certificates \
        curl \
        file \
        ftp \
        git \
        gnupg \
        iproute2 \
        iputils-ping \
        locales \
        lsb-release \
        nodejs \
        sudo \
        time \
        unzip \
        wget \
        zip

RUN apt-get install -y \
        clang \
        cmake \
        ninja-build \
        libc++-8-dev \
        libc++abi-8-dev
