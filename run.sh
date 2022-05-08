#!/bin/sh

PROG_NAME=ostinato
PROG_DIR=./bin/prog
PROG_PATH=$PROG_DIR/$PROG_NAME
if [ ! -f "$PROG_PATH" ]; then
        echo "$PROG_PATH does not exist"
        exit 1
fi
cd $PROG_DIR
./$PROG_NAME $*