# Docker Development Environment

This guide explains how to use the Docker development environment for fast local iteration on Linux without needing to install Qt or compilers directly on your host system.

## Overview

The container is built with Ubuntu 22.04, Ninja, CMake, and Qt 6.5.3 with `qtserialport`. The host workspace is mounted into `/workspace`, allowing you to edit files on the host and rebuild/run inside the container immediately.

## One-Time Setup

Allow local docker containers to connect to your X11 display server, then build the image:

```bash
xhost +local:docker && docker compose build
```

## Per-Change Development Loop

Whenever you make code changes, rebuild and launch the application using:

```bash
docker compose run --rm dev bash -c "cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel && ./build/EmbeddedUIDesigner"
```

The GUI window will appear directly on your host screen.
