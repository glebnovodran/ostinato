#!/bin/bash

CXX=${CXX:-g++}
TMP_DIR=../../tmp
XCORE_DIR=../../ext/crosscore

mkdir -p $TMP_DIR

$CXX -pthread -ggdb -ffast-math -ftree-vectorize -std=c++17 -I $XCORE_DIR $XCORE_DIR/crosscore.cpp bundle.cpp -o $TMP_DIR/bundle  $*
