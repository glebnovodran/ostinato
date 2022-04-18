#!/bin/sh

EXE_DIR=bin/prog
if [ ! -d "$EXE_DIR" ]; then
	mkdir -p $EXE_DIR
fi
EXE_NAME="ostinato"
EXE_PATH="$EXE_DIR/$EXE_NAME"

SRCS="`ls src/*.cpp` `ls crosscore/*.cpp`"
INCS="-I crosscore -I inc"
DEFS="-DX11"
LIBS="-lpthread -lX11"
BOLD_ON=""
BOLD_OFF=""
SYS_NAME="`uname -s`"
case $SYS_NAME in
	Linux)
		CXX=${CXX:-g++}
		LIBS="$LIBS -ldl"
		BOLD_ON="\e[1m"
		BOLD_OFF="\e[m"
	;;
	OpenBSD)
		CXX=${CXX:-clang++}
		INCS="$INCS -I/usr/X11R6/include"
		LIBS="$LIBS -L/usr/X11R6/lib"
	;;
	FreeBSD)
		CXX=${CXX:-clang++}
		INCS="$INCS -I/usr/local/include"
		LIBS="$LIBS -L/usr/local/lib"
	;;
	*)
		CXX=${CXX:-g++}
		echo "Warning: unknown system \"$SYS_NAME\", using defaults."
	;;
esac

echo "Compiling \"$BOLD_ON$EXE_PATH$BOLD_OFF\" for $BOLD_ON$SYS_NAME$BOLD_OFF with $BOLD_ON$CXX$BOLD_OFF."
rm -f $EXE_PATH
$CXX -ggdb -ffast-math -ftree-vectorize -std=c++11 $DEFS $INCS $SRCS -o $EXE_PATH $LIBS $*

echo -n "Build result: "
if [ -f "$EXE_PATH" ]; then
	echo "$BOLD_ON""Success""$BOLD_OFF!"
else
	echo "$BOLD_ON""Failure""$BOLD_OFF :("
fi 
