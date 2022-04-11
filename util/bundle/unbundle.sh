#!/bin/bash

EXE_DIR=$PWD
BIN_DIR=$EXE_DIR/../../bin
DAT_DIR=$BIN_DIR/dat
TMP_DIR=$EXE_DIR/../../tmp

OUT_DIR=$TMP_DIR/unbundled
mkdir -p $OUT_DIR

$TMP_DIR/bundle $BIN_DIR/ostinato.bnd -o:$OUT_DIR
