#!/bin/sh
if [ ! -d "./bin/linux" ]; then
	mkdir -p ./bin/linux
fi

CXX=${CXX:-g++}
$CXX -ggdb -ffast-math -ftree-vectorize -std=c++11 -I crosscore -I inc `ls src/*.cpp` `ls crosscore/*.cpp`  -o bin/linux/ostinato -ldl -lX11 -lpthread -DX11 $*