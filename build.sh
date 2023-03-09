#!/bin/sh

BUILD_TIMESTAMP="$(date -u)"

BOLD_ON="\e[1m"
UNDER_ON="\e[4m"
RED_ON="\e[31m"
GREEN_ON="\e[32m"
YELLOW_ON="\e[33m"
FMT_OFF="\e[0m"

SYS_NAME="`uname -s`"

if [ $SYS_NAME = Darwin ] || [ $SYS_NAME = FreeBSD ]; then
	BOLD_ON=""
	UNDER_ON=""
	RED_ON=""
	GREEN_ON=""
	YELLOW_ON=""
	FMT_OFF=""
fi


CROSSCORE_DIR="ext/crosscore"
VEMA_DIR="ext/vema"
BIN_DIR="bin"
DATA_DIR="$BIN_DIR/data"


if [ "$1" = "clean" ]; then
	printf "$BOLD_ON"
	printf "Cleaning... $FMT_OFF\n"
	rm -rdf $BIN_DIR
	rm -rdf ./ext
	rm -rdf ./tmp
	rm -rdf ./inc
	exit
fi

USE_WGET=0
USE_CURL=0
case $SYS_NAME in
	Darwin)
		USE_CURL=1
	;;
	*)
		if [ -x "`command -v wget`" ]; then
			USE_WGET=1
		elif [ -x "`command -v curl`" ]; then
			USE_CURL=1
		fi
	;;
esac

# dependencies
if [ ! -f "$CROSSCORE_DIR/crosscore.cpp" ]; then
	printf "$BOLD_ON$RED_ON""Downloading dependencies.""$FMT_OFF\n"
	cd util
	./get_crosscore.sh
	cd ..
fi

SRCS="`ls src/*.cpp` `ls $CROSSCORE_DIR/*.cpp`"
INCS="-I $CROSSCORE_DIR -I ext/inc -I inc"
DEFS=${OSTINATO_ALT_DEFS:-"-DX11"}
LIBS=${OSTINATO_ALT_LIBS:-"-lX11"}

# resources
RSRC_BASE="https://glebnovodran.github.io"
BUNDLE_URL="$RSRC_BASE/demo/ostinato.data"

OSTINATO_BND="$DATA_DIR/ostinato.bnd"
if [ ! -f "$BIN_DIR/bundle.off" ]; then
	if [ ! -f "$OSTINATO_BND" ]; then
		mkdir -p $DATA_DIR
		printf "$BOLD_ON$RED_ON""Downloading resources.""$FMT_OFF\n"
		if [ $USE_CURL -ne 0 ]; then
			curl "$BUNDLE_URL" -o "$OSTINATO_BND"
		elif [ $USE_WGET -ne 0 ]; then
			wget -O "$OSTINATO_BND" "$BUNDLE_URL"
		else
			printf "$RED_ON""Neither wget nor curl was found.""$FMT_OFF\n"
		fi
	fi
fi

# web-build
WEB_CC_SETUPMSG="To use emscripten: source <emsdk path>/emsdk_env.sh"
if [ -n "$EMSDK" ]; then
	WEB_CC="emcc -std=c++11"
	WEB_OPTS="-s ASSERTIONS=1 -s ALLOW_MEMORY_GROWTH=1 -DXD_THREADFUNCS_ENABLED=0"
	WEB_EMBED_OPT="-s SINGLE_FILE"
fi

if [ "$#" -gt 0 ]; then
	if [ "$1" = "wasm" ]; then
		if [ -z "$WEB_CC" ]; then
			printf "$RED_ON""WASM build requested, but web-compiler is missing.""$FMT_OFF\n"
			printf "$WEB_CC_SETUPMSG\n"
			exit 1
		fi
		shift
		WEB_OPTS="$WEB_OPTS $WEB_EMBED_OPT -s WASM=1"
		WEB_MODE="WebAssembly"
	elif [ "$1" = "js" ]; then
		if [ -z "$WEB_CC" ]; then
			printf "$RED_ON""JS build requested, but web-compiler is missing.""$FMT_OFF\n"
			printf "$WEB_CC_SETUPMSG\n"
			exit 1
		fi
		shift
		WEB_OPTS="$WEB_OPTS $WEB_EMBED_OPT -s WASM=0"
		WEB_MODE="JavaScript"
	elif [ "$1" = "wasm-0" ]; then
		if [ -z "$WEB_CC" ]; then
			printf "$RED_ON""Standalone WASM build requested, but web-compiler is missing.""$FMT_OFF\n"
			printf "$WEB_CC_SETUPMSG\n"
			exit 1
		fi
		shift
		WEB_OPTS="$WEB_OPTS -s WASM=1"
		WEB_MODE="WebAssembly (standalone)"
	elif [ "$1" = "js-0" ]; then
		if [ -z "$WEB_CC" ]; then
			printf "$RED_ON""Standalone JS build requested, but web-compiler is missing.""$FMT_OFF\n"
			printf "$WEB_CC_SETUPMSG\n"
			exit 1
		fi
		shift
		WEB_OPTS="$WEB_OPTS -s WASM=0"
		WEB_MODE="JavaScript (standalone)"
	fi
fi

if [ "$#" -gt 0 ]; then
	if [ "$1" = "vema" ]; then
		shift
		VEMA_URL="https://schaban.github.io/vema"
		mkdir -p $VEMA_DIR
		for vema in vema.c vema.h; do
			if [ ! -f $VEMA_DIR/$vema ]; then
				if [ $USE_CURL -ne 0 ]; then
					curl $VEMA_URL/$vema > $VEMA_DIR/$vema
				elif [ $USE_WGET -ne 0 ]; then
					wget -O "$VEMA_DIR/$vema" "$VEMA_URL/$vema"
				else
					printf "$RED_ON""Can't get \"$vema\".""$FMT_OFF\n"
				fi
			fi
		done
		cp $VEMA_DIR/vema.c $VEMA_DIR/vema.cpp
		VEMA_DEFS="-DXD_USE_VEMA -DVEMA_GCC_BUILTINS"
		VEMA_INCS="-I$VEMA_DIR"
		SRCS="$SRCS $VEMA_DIR/vema.cpp"
		DEFS="$DEFS $VEMA_DEFS"
		INCS="$INCS $VEMA_INCS"
	fi
fi

if [ -n "$WEB_MODE" ]; then
	WEB_EXPORTS_LIST=web/exports.txt
	OUT_HTML=bin/ostinato.html
	printf "Compiling $YELLOW_ON$OUT_HTML$FMT_OFF in $GREEN_ON$WEB_MODE$FMT_OFF mode.\n"
	WGL_OPTS="-s USE_SDL=2  -DOGLSYS_WEB"
	WEB_EXTS="--pre-js web/opt.js --shell-file web/shell.html --preload-file bin/data"
	WEB_EXPS="-s EXPORTED_RUNTIME_METHODS=ccall,cwrap -s EXPORTED_FUNCTIONS=_main"
	if [ -f $WEB_EXPORTS_LIST ]; then
		while read exp_fn; do
			WEB_EXPS="$WEB_EXPS,$exp_fn"
		done < $WEB_EXPORTS_LIST
	fi
	$WEB_CC $WEB_OPTS $WGL_OPTS $VEMA_DEFS $VEMA_INCS -I$CROSSCORE_DIR -O3 $SRCS $WEB_EXTS -o $OUT_HTML $WEB_EXPS $*
	sed -i 's/antialias:!1/antialias:1/g' $OUT_HTML
	if [ -n "$WEB_BUILD_TAG" ]; then
		WEB_BUILD_TAG=" [$WEB_BUILD_TAG]"
	fi
	TIMESTAMP_EXPR="s/~~ostinato-timestamp~~/~ $BUILD_TIMESTAMP ~$WEB_BUILD_TAG/g"
	sed -i "$TIMESTAMP_EXPR" $OUT_HTML
	exit
fi

# native build
EXE_DIR=bin/prog
if [ ! -d "$EXE_DIR" ]; then
	mkdir -p $EXE_DIR
fi
EXE_NAME="ostinato"
EXE_PATH="$EXE_DIR/$EXE_NAME"

SYS_KIND="generic"
SYS_OGL="Desktop"
EGL_LIBS=${OSTINATO_ALT_EGL_LIBS:-"-lEGL -lGLESv2"}
RPI_KIND_PATH="/sys/firmware/devicetree/base/model"
if [ -f $RPI_KIND_PATH ]; then
	SYS_KIND="`cat $RPI_KIND_PATH`"
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
	Darwin)
		CXX=${CXX:-clang++}
	;;
	SunOS)
		CXX=${CXX:g++}
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

if [ $SYS_NAME = Darwin ]; then
	if [ ! -d "tmp/obj" ]; then mkdir -p tmp/obj; fi
	MAC_MAIN=tmp/obj/mac_main.o
	MAC_LIBS="-framework OpenGL -framework Foundation -framework Cocoa"
	MAC_SRCS="$SRCS $MAC_MAIN"
	rm -f $MAC_MAIN
	clang -I $CROSSCORE_DIR/mac $CROSSCORE_DIR/mac/mac_main.m -c -o $MAC_MAIN
	$CXX -std=c++11 -ffast-math -ftree-vectorize $INCS -I $CROSSCORE_DIR/mac $MAC_SRCS -o $EXE_PATH $MAC_LIBS $*
else
	$CXX -std=c++11 -pthread -ggdb -ffast-math -ftree-vectorize $DEFS $INCS $SRCS -o $EXE_PATH $LIBS $*
fi

printf "Build result: "
if [ -f "$EXE_PATH" ]; then
	printf "$BOLD_ON$GREEN_ON""Success""$FMT_OFF!"
else
	printf "$BOLD_ON$RED_ON""Failure""$FMT_OFF :("
fi
echo ""
