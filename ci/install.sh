#!/bin/bash

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
   sudo apt-get install -y \
      libboost-all-dev \
      libgflags-dev \
      python3 \
      python3-pip \
      python3-setuptools \
      libsnappy-dev \
      zlib1g-dev \
      libbz2-dev \
      liblz4-dev \
      libzstd-dev \
      valgrind \
      virtualenv
elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
   brew install cmake \
      boost \
      openssl \
      python3 \
      zlib \
      snappy \
      bzip2 \
      gflags
fi

KOINOS_VE=$( $(dirname "$0")/find_ve.sh )

if [ ! -e "$KOINOS_VE" ]
then
    echo "Creating new virtualenv in $KOINOS_VE"
    virtualenv -p $(which python3) "$KOINOS_VE"
fi

"$KOINOS_VE/bin/pip3" install dataclasses-json Jinja2 importlib_resources pluginbase
