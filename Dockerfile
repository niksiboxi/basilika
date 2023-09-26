FROM python:3.8
LABEL version="1.0"

WORKDIR /tools

ENV GNU_INSTALL_ROOT=/tools/gcc-arm-none-eabi-10.3.2021.10/bin/
ENV GNU_VERSION=10.3.1

RUN apt update && \
    apt upgrade -y \
    apt install -y \
        astyle \
        build-essential \
        cppcheck && \
        gcc-multilib g++-multilib \
        git \
        make \
        ruby-full \
        wget \
    apt clean

RUN gem install ceedling
RUN curl --show-error --silent -L  --output gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz
RUN tar xvf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz
RUN rm gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz