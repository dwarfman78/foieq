FROM alpine:3.19.0
RUN apk update && \
    apk add --no-cache \
    libstdc++ \
    build-base \
    cmake \
    openssl-dev \
    asio-dev

WORKDIR /foieq

COPY include/*.hpp ./
COPY *.cpp ./
COPY lib ./lib
COPY templates ./templates
COPY static ./static
COPY CMakeLists.txt ./

WORKDIR /foieq/lib/Crow/build
RUN cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF
RUN make install

WORKDIR /foieq
RUN cmake .

RUN make 
ENV CONFIG_JSON=config.json
CMD ./foieq ${CONFIG_JSON}
