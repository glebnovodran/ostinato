#!/bin/sh
if [ ! -d "crosscore" ]; then mkdir -p crosscore; fi
for xcore in crosscore.hpp crosscore.cpp demo.hpp demo.cpp draw.hpp oglsys.hpp oglsys.cpp oglsys.inc scene.hpp scene.cpp smprig.hpp smprig.cpp draw_ogl.cpp
do
	wget -O crosscore/$xcore https://raw.githubusercontent.com/schaban/crosscore_dev/main/src/$xcore
done

if [ ! -d "crosscore/ogl" ]; then mkdir -p crosscore/ogl; fi
for xcore in gpu_defs.h progs.inc shaders.inc
do
	wget -O crosscore/ogl/$xcore https://raw.githubusercontent.com/schaban/crosscore_dev/main/src/ogl/$xcore
done

for xcore in get_egl_headers.sh get_gles_headers.sh get_ogl_headers.sh get_headers.sh
do
	wget -O $xcore https://raw.githubusercontent.com/schaban/crosscore_dev/main/$xcore
done
