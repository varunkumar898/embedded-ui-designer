FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential cmake ninja-build git python3 python3-pip \
    libgl1-mesa-dev libxkbcommon-x11-0 libxcb-cursor0 libxcb-icccm4 \
    libxcb-image0 libxcb-keysyms1 libxcb-render-util0 libxcb-xinerama0 \
    && rm -rf /var/lib/apt/lists/*
RUN pip3 install aqtinstall && \
    python3 -m aqt install-qt linux desktop 6.5.3 gcc_64 -m qtserialport -O /opt/Qt
ENV PATH="/opt/Qt/6.5.3/gcc_64/bin:${PATH}"
ENV CMAKE_PREFIX_PATH="/opt/Qt/6.5.3/gcc_64"
WORKDIR /workspace
