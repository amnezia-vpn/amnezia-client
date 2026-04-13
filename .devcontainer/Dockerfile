FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Basic build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    unzip \
    p7zip-full \
    ca-certificates \
    libgl1-mesa-dev \
    libgstreamer-gl1.0-0 \
    libpulse-dev \
    libsecret-1-dev \
    libxcb-glx0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-render0 \
    libxcb-shape0 \
    libxcb-shm0 \
    libxcb-sync1 \
    libxcb-util1 \
    libxcb-xfixes0 \
    libxcb-xinerama0 \
    libxcb1 \
    libxkbcommon-dev \
    libxkbcommon-x11-0 \
    libxcb-xkb-dev \
    libfontconfig1 \
    libfreetype6 \
    libx11-6 \
    libx11-xcb1 \
    libxext6 \
    libxfixes3 \
    libxi6 \
    libxrender1 \
    libxslt1.1 \
    libxkbcommon0 \
    libdbus-1-3 \
    libnss3 \
    libatk1.0-0 \
    libatk-bridge2.0-0 \
    libcups2 \
    libdrm2 \
    libgbm1 \
    libasound2t64 \
    libpango-1.0-0 \
    libcairo2 \
    && rm -rf /var/lib/apt/lists/*

# Install aqtinstall for Qt
RUN apt-get update && apt-get install -y python3 python3-pip && \
    pip3 install --break-system-packages aqtinstall && \
    rm -rf /var/lib/apt/lists/*

# Install Qt 6.6.2 (matching build_linux.sh default)
ARG QT_VERSION=6.6.2
ARG QT_ARCH=gcc_64
ARG QT_DIR=/opt/qt

RUN aqt install-qt --outputdir ${QT_DIR} linux desktop ${QT_VERSION} ${QT_ARCH} --modules \
    qtremoteobjects \
    qt5compat \
    qtshadertools && \
    rm -rf /tmp/* /var/tmp/*

ENV QT_BIN_DIR=${QT_DIR}/${QT_VERSION}/${QT_ARCH}/bin
ENV PATH=${QT_BIN_DIR}:${PATH}

# Set default locale
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

WORKDIR /workspace
