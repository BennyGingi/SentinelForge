# syntax=docker/dockerfile:1

# ---- Builder stage ----------------------------------------------------------
# Ubuntu image with a C++ toolchain. Only present to compile the binary;
# discarded after the build (nothing from this stage ships in the final image).
FROM ubuntu:24.04 AS builder

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        cmake \
        g++ \
        make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY collector-cpp/ ./collector-cpp/

RUN cmake -S collector-cpp -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --config Release

# ---- Runtime stage -----------------------------------------------------------
# Fresh Ubuntu base with no compiler and no source: only the shared library the
# binary needs (libstdc++6) plus the executable itself.
FROM ubuntu:24.04 AS runtime

RUN apt-get update \
    && apt-get install -y --no-install-recommends libstdc++6 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --no-create-home --shell /usr/sbin/nologin collector

COPY --from=builder --chown=collector:collector /src/build/collector /usr/local/bin/collector

USER collector

ENTRYPOINT ["/usr/local/bin/collector"]
