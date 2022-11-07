FROM ubuntu:22.04
MAINTAINER  huangzhenming@canaan-creative.com

# Add ubuntu source
ADD sources.list /etc/apt/sources.list
# Add k210 toolchain and some related tools
RUN DEBIAN_FRONTEND=noninteractive apt-get update -qq \
    && DEBIAN_FRONTEND=noninteractive apt-get install -yq \
    && DEBIAN_FRONTEND=noninteractive apt-get install -yq tzdata \
        build-essential \
        git \
        git-lfs \
        wget \
        unzip \
        rsync \
        iputils-ping \
        cmake \
        python3 \
        python3-pip \
    && pip3 install paramiko pyyaml pyserial \
    && update-alternatives --install /usr/bin/python python /usr/bin/python3 10 \
    && wget https://github.com/kendryte/kendryte-gnu-toolchain/releases/download/v8.2.0-20190409/kendryte-toolchain-ubuntu-amd64-8.2.0-20190409.tar.xz \
    && tar -Jxf kendryte-toolchain-ubuntu-amd64-8.2.0-20190409.tar.xz -C /opt \
    && rm -f kendryte-toolchain-ubuntu-amd64-8.2.0-20190409.tar.xz \
    && DEBIAN_FRONTEND=noninteractive apt-get autoremove -y --purge \
    && DEBIAN_FRONTEND=noninteractive apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf /tmp
# Set environment variables
ENV PATH="$PATH:/opt/kendryte-toolchain/bin"
# Set the working directory, you may start your firmware development from here
WORKDIR /workspace