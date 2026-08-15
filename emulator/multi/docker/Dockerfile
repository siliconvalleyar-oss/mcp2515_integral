# ECU Multi-Emulator Docker Image
FROM debian:bookworm-slim AS builder

LABEL maintainer="OpenCode ECU Emulator Project"
LABEL description="Multi-Brand Vehicle Diagnostic ECU Emulator"

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libsqlite3-dev \
    libnl-3-dev \
    libnl-route-3-dev \
    nlohmann-json3-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copy source
COPY . .

# Build
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc) && \
    make install

# Runtime stage
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    libsqlite3-0 \
    libnl-3-200 \
    libnl-route-3-200 \
    can-utils \
    iproute2 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/bin/ecu_emulator /usr/local/bin/
COPY --from=builder /etc/ecu_emulator /etc/ecu_emulator

# Create log directory
RUN mkdir -p /var/log && \
    mkdir -p /etc/ecu_emulator

EXPOSE 8080 8081 8082

# Entry point
COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["ecu_emulator", "/etc/ecu_emulator/config.json"]
