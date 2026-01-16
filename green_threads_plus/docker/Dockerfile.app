FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libmicrohttpd-dev \
    libwebsockets-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN rm -rf build && mkdir build && cd build && cmake .. && make

# Expose HTTP and WebSocket ports
EXPOSE 8080 8081

CMD ["./build/green_threads_app", "green"]
