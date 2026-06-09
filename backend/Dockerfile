FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libboost-dev \
    libboost-json-dev \
    libboost-url-dev \
    libcurl4-openssl-dev \
    libpq-dev \
    libpqxx-dev \
    libargon2-dev \
    nlohmann-json3-dev \
    libssl-dev \
    libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app/backend

COPY . .

RUN cmake -S . -B build \
    && cmake --build build -j$(nproc)

EXPOSE 8080 9100

CMD ["./build/chronos_backend", "0.0.0.0", "8080"]
