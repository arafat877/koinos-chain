#!/bin/bash

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
   sudo apt-get install -y \
      libboost-all-dev \
      libgflags-dev \
      python3 \
      python3-pip \
      python3-setuptools \
      python3-venv \
      libsnappy-dev \
      zlib1g-dev \
      libbz2-dev \
      liblz4-dev \
      libzstd-dev \
      valgrind
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
    python3 -m venv "$KOINOS_VE"
fi

"$KOINOS_VE/bin/pip" install wheel
"$KOINOS_VE/bin/pip" install dataclasses-json Jinja2 importlib_resources pluginbase
