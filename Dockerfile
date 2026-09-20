# Build environment for the talking disk (everything except the QEMU-based disk assembly, which needs KVM;
# run/mktalkdisk.sh works inside the container only with TCG, slowly). Usage:
#   docker build -t vintage-pc-speech . && docker run --rm -v $PWD:/work vintage-pc-speech ./release.sh
FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential curl ca-certificates unzip zip xz-utils bzip2 python3 nasm qemu-system-x86 socat xorriso ffmpeg git \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /work
COPY run/get-toolchains.sh /tmp/get-toolchains.sh
RUN TOOLS=/opt/tools bash /tmp/get-toolchains.sh
ENV TOOLS=/opt/tools WATCOM=/opt/tools/ow DJ=/opt/tools/djgpp/bin
