#!/bin/sh

BOLD_ON="\e[1m"
UNDER_ON="\e[4m"
RED_ON="\e[31m"
GREEN_ON="\e[32m"
YELLOW_ON="\e[33m"
FMT_OFF="\e[0m"
CROSSCORE_DIR="ext/crosscore"

# dependencies
if [ ! -f "$CROSSCORE_DIR/crosscore.cpp" ]; then
	printf "$BOLD_ON$RED_ON""Downloading dependencies.""$FMT_OFF\n"
	cd util
	./get_crosscore.sh
	cd ..
fi

SRCS="`ls src/*.cpp` `ls $CROSSCORE_DIR/*.cpp`"
INCS="-I $CROSSCORE_DIR -I ext/inc -I inc"

# resources
RSRC_BASE=https://github.com/glebnovodran/glebnovodran.github.io/raw/main

OSTINATO_BND="bin/data/ostinato.bnd"
if [ ! -f "$OSTINATO_BND" ]; then
	mkdir -p bin/data
	printf "$BOLD_ON$RED_ON""Downloading resources.""$FMT_OFF\n"
	wget -O "$OSTINATO_BND" $RSRC_BASE/demo/ostinato.data
fi

# web-build
if [ -n "$EMSDK" ]; then
	WEB_CC="emcc -std=c++11"
	WEB_OPTS="-s ASSERTIONS=1 -s ALLOW_MEMORY_GROWTH=1"
	WEB_OPTS="$WEB_OPTS -s SINGLE_FILE"
fi

if [ "$#" -gt 0 ]; then
	if [ "$1" = "wasm" ]; then
		if [ -z "$WEB_CC" ]; then
			printf "$RED_ON""WASM build requested, but web-compiler is missing.""$FMT_OFF\n"
			exit 1
		fi
		shift
		WEB_OPTS="$WEB_OPTS -s WASM=1"
		WEB_MODE="WebAssembly"
	elif [ "$1" = "js" ]; then
	        if [ -z "$WEB_CC" ]; then
			echo "$RED_ON""JS build requested, but web-compiler is missing.""$FMT_OFF\n"
			exit 1
		fi
		shift
		WEB_OPTS="$WEB_OPTS -s WASM=0"
		WEB_MODE="JavaScript"
	fi
fi

if [ -n "$WEB_MODE" ]; then
	OUT_HTML=bin/ostinato.html
	printf "Compiling $YELLOW_ON$OUT_HTML$FMT_OFF in $GREEN_ON$WEB_MODE$FMT_OFF mode.\n"
	WGL_OPTS="-s USE_SDL=2  -DOGLSYS_WEB"
	WEB_EXTS="--pre-js web/opt.js --shell-file web/shell.html --preload-file bin/data"
	$WEB_CC $WEB_OPTS $WGL_OPTS -I $CROSSCORE_DIR -O3 $SRCS $WEB_EXTS -o $OUT_HTML -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' -s EXPORTED_FUNCTIONS='["_main"]'
	sed -i 's/antialias:!1/antialias:1/g' $OUT_HTML
	exit
fi

# native build
EXE_DIR=bin/prog
if [ ! -d "$EXE_DIR" ]; then
	mkdir -p $EXE_DIR
fi
EXE_NAME="ostinato"
EXE_PATH="$EXE_DIR/$EXE_NAME"

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
