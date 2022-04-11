#!/bin/bash
if [ ! -d "./bin/linux" ]; then
	mkdir -p ./bin/linux
fi

g++ -ggdb -ffast-math -ftree-vectorize -std=c++11 -I crosscore -I inc `ls src/*.cpp` `ls crosscore/*.cpp`  -o bin/linux/ostinato -ldl -lX11 -lpthread -DX11 $*