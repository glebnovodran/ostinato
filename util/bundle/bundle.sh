#!/bin/bash

EXE_DIR=$PWD
BIN_DIR=$EXE_DIR/../../bin
DAT_DIR=$BIN_DIR/data
TMP_DIR=$EXE_DIR/../../tmp
cd $DAT_DIR
ls -1d */* | $TMP_DIR/bundle -o:$BIN_DIR/ostinato.bnd
cd $EXE_DIR
