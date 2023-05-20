FROM gitpod/workspace-full-vnc
                    
USER gitpod

RUN sudo apt-get -q update \
    && sudo apt-get install -yq \
        build-essential \
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
        p7zip-full \
    && sudo rm -rf /var/lib/apt/lists/*

RUN pip3 install aqtinstall

ARG QT_VERSION=6.4.1
ARG QT_ARCH=gcc_64

ARG QT_DIR=/workspace/qt
RUN aqt install-qt --outputdir ${QT_DIR} linux desktop ${QT_VERSION} ${QT_ARCH} --modules \
    qtremoteobjects \
    qt5compat \
    qtshadertools
ENV QT_BIN_DIR=${QT_DIR}/${QT_VERSION}/${QT_ARCH}/bin

ARG QIF_VERSION=4.5
ARG QIF_DIR=/workspace/qif
RUN aqt install-tool --outputdir ${QIF_DIR} linux desktop tools_ifw
ENV QIF_BIN_DIR=${QIF_DIR}/Tools/QtInstallerFramework/${QIF_VERSION}/bin
