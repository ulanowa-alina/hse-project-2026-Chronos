FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential cmake git pkg-config \
    qt6-base-dev libpq-dev libpqxx-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt .

COPY backend ./backend

RUN mkdir build && cd build \
    && cmake .. \
    && make

CMD ["./build/chronos_backend"]
