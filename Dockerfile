# Stage 1: Build
FROM alpine:latest AS builder
RUN apk update && apk add --no-cache \
    build-base \
    g++ \
    cmake \
    boost-dev \
    hiredis-dev

WORKDIR /test
COPY src/ ./src
COPY CMakeLists.txt .
RUN mkdir -p build && \
    cd build && \
    cmake -DCMAKE_CXX_STANDARD=20 .. && \
    make -j

# Stage 2: Runtime
FROM alpine:latest
RUN apk add --no-cache \
    libstdc++
    
WORKDIR /test
RUN mkdir log
COPY --from=builder /test/build/src/rtmp_server/rtmp_server .
EXPOSE 1935 7775
CMD ["./rtmp_server"]
