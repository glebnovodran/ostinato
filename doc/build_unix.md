# Unux build

Ostinato can be built for Linux (including Raspberry Pi OS), OpenBSD or FreeBSD systems.
## Cloning

`git clone --depth 1 https://github.com/glebnovodran/ostinato.git`

## Dependencies

The build depends on [crosscore](https://github.com/schaban/crosscore_dev) library sources. build.sh detects presence of the crosscore sources. If not present the build script downloads them automatically through executing util/get_crosscore.sh.

Also, the build depends on OGL/EGL/GLES header files. If not present, they are downloaded by the build script too.

Some systems may not have some necessary tools/libraries preinstalled.
For RPi OS the C++ compiler toolchain should be installed

`sudo apt install build-essential`

along with X11 developnment headers

`sudo apt install libx11-dev`

For FreeBSD and OpenBSD it is necessary to install _wget_:

OpenBSD: `pkg_add git wget`

FreeBSD: `pkg install git wget`


## Build command:

`./build.sh <compiler options>`

By default Ostinato is built for debugging. To build an optimized version add `-O3 -flto -march=native` compiler options.

## OGL/GLES

By defaul Ostinato is built for desktop. To build it for GLES it is necessary to set OGL_MODE environment variable value:

`OGL_MODE=GLES ./build.sh`

**Note:** For a Raspberry Pi build GLES is selected automatically.
