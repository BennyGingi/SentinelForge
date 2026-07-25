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

# Only the `collector` target -- collector_tests/regression_runner (and the
# googletest fetch their subdirectories trigger at configure time) aren't
# needed to run the collector and would only slow the image build down.
RUN cmake -S collector-cpp -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --config Release --target collector --parallel

# ---- Runtime stage -----------------------------------------------------------
# Fresh Ubuntu base with no compiler and no source: only the shared library the
# binary needs (libstdc++6) plus the executable itself.
FROM ubuntu:24.04 AS runtime

RUN apt-get update \
    && apt-get install -y --no-install-recommends libstdc++6 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --no-create-home --shell /usr/sbin/nologin collector

COPY --from=builder --chown=collector:collector /src/build/collector /usr/local/bin/collector

# Configuration::LoadFromFile is looked up at a path CMake bakes in at
# configure time (CONFIG_FILE_PATH = <source-dir>/../config/sentinelforge.json,
# i.e. /src/config/sentinelforge.json for this build). Relative paths inside
# that file resolve against its grandparent (/src), which is also where the
# DEFAULT_* fallback macros point -- so mirroring collector-cpp's repo layout
# under /src here makes the compiled-in paths line up whether or not the
# config file is actually found. api.bind_address is 0.0.0.0 here (not the
# repo's 127.0.0.1 default) because the host can only reach a container's API
# through a bind that isn't localhost-only.
COPY --chown=collector:collector docker/collector.config.json /src/config/sentinelforge.json

# rules/ and sigma-rules/ are meant to be bind-mounted read-only over these
# (see docker-compose.yml); events/incoming is bind-mounted read-write so the
# host can drop event files in. processed/, failed/, output/, and logs/ are
# purely container-internal and never mounted.
RUN mkdir -p /src/rules /src/sigma-rules \
             /src/events/incoming /src/events/processed /src/events/failed \
             /src/output /src/logs \
    && chown -R collector:collector /src

USER collector
WORKDIR /src

EXPOSE 8787

ENTRYPOINT ["/usr/local/bin/collector"]
