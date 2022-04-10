#!/bin/sh
if [ ! -d "crosscore" ]; then mkdir -p crosscore; fi
for xcore in crosscore.hpp crosscore.cpp demo.hpp demo.cpp draw.hpp oglsys.hpp oglsys.cpp oglsys.inc scene.hpp scene.cpp smprig.hpp smprig.cpp
do
	wget -P crosscore https://raw.githubusercontent.com/schaban/crosscore_dev/main/src/$xcore
done
