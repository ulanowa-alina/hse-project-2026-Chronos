FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libboost-dev \
    libpq-dev \
    libpqxx-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app/backend

COPY . .

RUN cmake -S . -B build \
    && cmake --build build -j$(nproc)

EXPOSE 8080

CMD ["./build/chronos_backend", "0.0.0.0", "8080"]
