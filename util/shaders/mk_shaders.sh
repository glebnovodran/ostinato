#!/bin/sh

TMP_DIR=../../tmp/shaders
mkdir -p $TMP_DIR

OGL_DIR=$TMP_DIR/ogl
OGL_FP32_DIR=$TMP_DIR/ogl_fp32

WK_DIR=$PWD

if [ ! -d $OGL_DIR ]; then
	cd $TMP_DIR
	ARC_URL="https://schaban.github.io/crosscore_web_demo/xcore_web.tar.xz"
	ARC_DATA_TMP="xcore_web"
	ARC_DATA_BIN="$ARC_DATA_TMP/bin"
	ARC_DATA_DIR="$ARC_DATA_BIN/data"
	ARC_DATA_OGL="$ARC_DATA_DIR/ogl"
	wget -q -O - $ARC_URL | (tar --strip-components=3 -xvJf - $ARC_DATA_OGL)
	cd $WK_DIR

	printf "Adding tags..."
	for glsl in `ls $OGL_DIR/*`; do
		echo "// [`basename $glsl`]" >> $glsl
	done
fi

printf "Patching exposure checks...\n"
for glsl in `ls $OGL_DIR/*`; do
	echo $glsl
	sed -i "s/all(greaterThan(e, vec3(0.0)))/true/g" $glsl
done

mkdir -p $OGL_FP32_DIR
printf "Making fp32 variant...\n"
cp -fa $OGL_DIR/. $OGL_FP32_DIR
for glsl in `ls $OGL_FP32_DIR/*`; do
	echo $glsl
	sed -i "s/precision mediump float;/precision highp float;/g" $glsl
	sed -i "s/define HALF mediump/define HALF highp/g" $glsl
done
