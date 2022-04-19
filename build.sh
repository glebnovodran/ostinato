#!/bin/sh

BOLD_ON="\e[1m"
UNDER_ON="\e[4m"
RED_ON="\e[31m"
GREEN_ON="\e[32m"
YELLOW_ON="\e[33m"
FMT_OFF="\e[0m"
if [ ! -f "crosscore/crosscore.cpp" ]; then
	printf "$BOLD_ON$RED_ON""Downloading dependencies.""$FMT_OFF\n"
	./get_crosscore.sh
fi

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
SYS_NAME="`uname -s`"
case $SYS_NAME in
	Linux)
		CXX=${CXX:-g++}
		LIBS="$LIBS -ldl"
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

printf "Compiling \"$BOLD_ON$YELLOW_ON$UNDER_ON$EXE_PATH$FMT_OFF\" for $BOLD_ON$SYS_NAME$FMT_OFF with $BOLD_ON$CXX$FMT_OFF.\n"
rm -f $EXE_PATH
$CXX -ggdb -ffast-math -ftree-vectorize -std=c++11 $DEFS $INCS $SRCS -o $EXE_PATH $LIBS $*

echo -n "Build result: "
if [ -f "$EXE_PATH" ]; then
	printf "$BOLD_ON$GREEN_ON""Success""$FMT_OFF!"
else
	printf "$BOLD_ON$RED_ON""Failure""$FMT_OFF :("
fi
echo ""
