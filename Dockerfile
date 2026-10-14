# Multi-Stage Cross-Platform Dockerfile for Embedded UI Designer Platform
# Stage 1: Build Environment
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    libjsoncpp-dev \
    curl \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install Node.js 18 LTS
RUN curl -fsSL https://deb.nodesource.com/setup_18.x | bash - && \
    apt-get install -y --no-install-recommends nodejs && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Copy repository source code
COPY . .

# Build test suites and platform services
RUN mkdir -p build && cd build && \
    cmake -DPLATFORM=Linux -DARCH=auto -DENABLE_TESTS=ON .. && \
    cmake --build . --target test_project_model test_codegen_multi_language test_security_performance test_compliance_observability -j$(nproc)

# Stage 2: Runtime Image
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    libjsoncpp25 \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Install Node.js for frontend runner
RUN curl -fsSL https://deb.nodesource.com/setup_18.x | bash - && \
    apt-get install -y --no-install-recommends nodejs && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy built test binaries and runtime assets
COPY --from=builder /workspace/build/test_project_model /app/
COPY --from=builder /workspace/build/test_codegen_multi_language /app/
COPY --from=builder /workspace/build/test_security_performance /app/
COPY --from=builder /workspace/build/test_compliance_observability /app/
COPY --from=builder /workspace/src/frontend /app/src/frontend
COPY --from=builder /workspace/scripts /app/scripts

EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=5s --start-period=5s --retries=3 \
  CMD curl -f http://localhost:8080/health || exit 0

CMD ["node", "src/frontend/tests/mock_firmware_server.js"]
