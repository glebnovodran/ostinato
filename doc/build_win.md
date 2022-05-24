# Windows build
To build Ostinato for Windows it is necessary you will need to install [tdm-gcc](https://jmeubank.github.io/tdm-gcc/download/)

## Cloning

`git clone --depth 1 https://github.com/glebnovodran/ostinato.git`

## Dependencies

The build depends on [crosscore](https://github.com/schaban/crosscore_dev) library sources. build.bat detects presence of the crosscore sources. If not present the build script downloads them automatically.

The build depends on OGL/EGL header files. If not already present, they will be downloaded automatically.

Build command:

`set TDM_HOME=<path_to_tdm>`

`build.bat`
