#!/bin/sh

BOLD_ON="\e[1m"
UNDER_ON="\e[4m"
RED_ON="\e[31m"
GREEN_ON="\e[32m"
YELLOW_ON="\e[33m"
FMT_OFF="\e[0m"
if [ ! -f "ext/crosscore/crosscore.cpp" ]; then
	printf "$BOLD_ON$RED_ON""Downloading dependencies.""$FMT_OFF\n"
	cd util
	./get_crosscore.sh
	cd ..
fi

RSRC_BASE=https://github.com/glebnovodran/glebnovodran.github.io/raw/main

OSTINATO_BND="bin/data/ostinato.bnd"
if [ ! -f "$OSTINATO_BND" ]; then
	mkdir -p bin/data
	printf "$BOLD_ON$RED_ON""Downloading resources.""$FMT_OFF\n"
	wget -O "$OSTINATO_BND" $RSRC_BASE/demo/ostinato.data
fi

EXE_DIR=bin/prog
if [ ! -d "$EXE_DIR" ]; then
	mkdir -p $EXE_DIR
fi
EXE_NAME="ostinato"
EXE_PATH="$EXE_DIR/$EXE_NAME"

SRCS="`ls src/*.cpp` `ls ext/crosscore/*.cpp`"
INCS="-I ext/crosscore -I ext/inc -I inc"
DEFS="-DX11"
LIBS="-lpthread -lX11"
SYS_NAME="`uname -s`"
SYS_KIND="generic"
SYS_OGL="Desktop"
EGL_LIBS="-lEGL -lGLESv2"
RPI_KIND_PATH="/sys/firmware/devicetree/base/model"
if [ -f $RPI_KIND_PATH ]; then
	SYS_KIND="`cat $RPI_KIND_PATH)`"
fi
case $SYS_NAME in
	Linux)
		CXX=${CXX:-g++}
		LIBS="$LIBS -ldl"
		case $SYS_KIND in
			Raspberry*)
				SYS_OGL="GLES"
				LIBS="$LIBS -Llib"
				if [ ! -d "lib" ]; then mkdir -p lib; fi
				case `uname -m` in
					armv7l)
						libDir=/lib/arm-linux-gnueabihf
					;;
					*)
						libDir=/lib/`uname -m`-linux-gnu
					;;
				esac
				for lib in X11 EGL GLESv2
				do
					libBase=$libDir/lib$lib.so
					libPath=`ls -1 $libBase.[0-9]*.* | head -1`
					lnkPath=./lib/lib$lib.so
					if [ ! -f "$lnkPath" ]; then
						echo "Making link to" $libPath
						ln -s $libPath $lnkPath
					fi
				done
			;;
			*)
			;;
		esac
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

OGL_MODE=${OGL_MODE:-$SYS_OGL}
case $OGL_MODE in
	GLES)
		LIBS="$LIBS $EGL_LIBS"
		DEFS="$DEFS -DOGLSYS_ES=1"
	;;
esac

printf "Compiling \"$BOLD_ON$YELLOW_ON$UNDER_ON$EXE_PATH$FMT_OFF\" for $BOLD_ON$SYS_NAME$FMT_OFF with $BOLD_ON$CXX$FMT_OFF.\n"
printf "OGL mode: $BOLD_ON$OGL_MODE$FMT_OFF.\n"
rm -f $EXE_PATH
$CXX -ggdb -ffast-math -ftree-vectorize -std=c++11 $DEFS $INCS $SRCS -o $EXE_PATH $LIBS $*

echo -n "Build result: "
if [ -f "$EXE_PATH" ]; then
	printf "$BOLD_ON$GREEN_ON""Success""$FMT_OFF!"
else
	printf "$BOLD_ON$RED_ON""Failure""$FMT_OFF :("
fi
echo ""
