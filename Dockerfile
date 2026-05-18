FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++-13 cmake ninja-build \
    && rm -rf /var/lib/apt/lists/*

ENV CXX=g++-13

WORKDIR /workspace
COPY . .

RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build -j$(nproc)

RUN ctest --test-dir build -j$(nproc) --output-on-failure

ENTRYPOINT ["./build/bittorrent"]
