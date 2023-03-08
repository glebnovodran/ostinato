# Unux build

Ostinato can be built for Linux (including Raspberry Pi OS), OpenBSD or FreeBSD systems.

### Cloning

`git clone --depth 1 https://github.com/glebnovodran/ostinato.git`

### Dependencies

The build depends on [-=crosscore=-](https://github.com/schaban/crosscore_dev) library sources. build.sh detects presence of the crosscore sources. If the sources are not present, it downloads them automatically through executing util/get_crosscore.sh.

Also, the build depends on OGL/EGL/GLES header files. If not present, they are downloaded by the build script too.

Some systems may not have some necessary tools/libraries pre-installed:

##### **_C++ compiler toolchain_**

Ubintu-style distribuntions, RPi OS:
`sudo apt install build-essential`

Fedora Linux has C-compiler preinstalled, but C++ is not, so it is necessary to execute:
`sudo dnf install g++`

##### **_X11 development headers_**
Ubintu-style distribuntions, RPi OS:
`sudo apt install libx11-dev`

Fedora:
`sudo dnf install libX11-devel`

##### **_wget_**
For FreeBSD and OpenBSD it is necessary to install **_wget_**:

OpenBSD: `pkg_add git wget`

FreeBSD: `pkg install git wget`


### Build command:

`./build.sh <compiler options>`

By default Ostinato is built for debugging. To build an optimized version add `-O3 -flto -march=native` compiler options.

### Build for OpenGL or OpenGL ES

By default Ostinato is built for desktop OpenGL. To build it for OpenGL ES it is necessary to set OGL_MODE environment variable value accordingly:

`OGL_MODE=GLES ./build.sh`

**Note:** GLES mode is selected automatically for a Raspberry Pi build.
