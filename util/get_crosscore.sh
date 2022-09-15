#!/bin/sh

cd ..
if [ ! -d "ext" ]; then mkdir -p ext; fi
if [ ! -d "ext/crosscore" ]; then mkdir -p ext/crosscore; fi
if [ ! -d "ext/crosscore/ogl" ]; then mkdir -p ext/crosscore/ogl; fi

XCORE_URL="https://raw.githubusercontent.com/schaban/crosscore_dev/main/src"
XCORE_MAIN="crosscore.hpp crosscore.cpp demo.hpp demo.cpp draw.hpp oglsys.hpp oglsys.cpp oglsys.inc scene.hpp scene.cpp smprig.hpp smprig.cpp draw_ogl.cpp"
XCORE_OGL="gpu_defs.h progs.inc shaders.inc"

if [ "`uname -s`" = "Darwin" ]
then
	if [ ! -d "ext/crosscore/mac" ]; then mkdir -p ext/crosscore/mac; fi
	for xcore in $XCORE_MAIN
	do
		curl $XCORE_URL/$xcore -o ext/crosscore/$xcore
	done
	for xcore in $XCORE_OGL
	do
		curl $XCORE_URL/ogl/$xcore -o ext/crosscore/ogl/$xcore
	done
	for xcore in mac_main.m mac_ifc.h
	do
		curl $XCORE_URL/mac/$xcore -o ext/crosscore/mac/$xcore
	done
	exit
fi

for xcore in $XCORE_MAIN
do
	wget -O ext/crosscore/$xcore $XCORE_URL/$xcore
done

for xcore in $XCORE_OGL
do
	wget -O ext/crosscore/ogl/$xcore $XCORE_URL/ogl/$xcore
done

for xcore in get_egl_headers.sh get_gles_headers.sh get_ogl_headers.sh
do
	wget -q -O - https://raw.githubusercontent.com/schaban/crosscore_dev/main/$xcore | sh
done
