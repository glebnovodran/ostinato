# Unux build

Ostinato can be built for Linux (including Raspberry Pi OS), OpenBSD or FreeBSD systems.

Build command:

`./build.sh <compiler options>`


The build depends on [crosscore](https://github.com/schaban/crosscore_dev) library sources. build.sh detects presence of the crosscore sources. If not present the build script downloads them automatically through executing util/get_crosscore.sh.

Ostainato can be build for either desktop OpenGL or for OpenGL GLES. By default it is built for desktop. To build it for GLES it is necessary to set OGL_MODE environment variable value:

`export OGL_MODE=GLES && ./build.sh`

**Note:** For a Raspberry Pi build GLES is selected automatically
