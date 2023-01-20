#!/bin/sh

cmake \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_PREFIX_PATH=$HOME/nestdaq \
    -DCMAKE_INSTALL_PREFIX=$HOME/nestdaq \
    -Dboost_PREFIX=$HOME/nestdaq \
    -B ./build \
    -S .
