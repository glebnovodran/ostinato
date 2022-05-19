@echo off

setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

set PROG_NAME=ostinato

set SRC_DIR=src
set BIN_DIR=bin
set TMP_DIR=tmp
set EXT_DIR=ext
set INC_DIR=%EXT_DIR%\inc
set DATA_DIR=%BIN_DIR%\data
set PROG_DIR=%BIN_DIR%\prog
set XCORE_DIR=%EXT_DIR%\crosscore
set BUNDLE_NAME=%PROG_NAME%.bnd
set PROG_PATH=%PROG_DIR%\%PROG_NAME%.exe
set BUNDLE_PATH=%DATA_DIR%\%BUNDLE_NAME%

if not exist %BIN_DIR% mkdir %BIN_DIR%
if not exist %PROG_DIR% mkdir %PROG_DIR%
if not exist %DATA_DIR% mkdir %DATA_DIR%

set BND_URL=https://glebnovodran.github.io/demo/ostinato.data

echo   ~ Compiling Ostinato for Windows ~

goto :start

:mk_hget
	echo /* WSH HTTP downloader */ > %_HGET_%
	echo var req = new ActiveXObject("MSXML2.XMLHTTP.6.0"); >> %_HGET_%
	echo var url = WScript.Arguments(0); var res = null; >> %_HGET_%
	echo WScript.StdErr.WriteLine("Downloading " + url + "..."); >> %_HGET_%
	echo try{req.Open("GET",url,false);req.Send();if(req.Status==200)res=req.ResponseText; >> %_HGET_%
	echo } catch (e) { WScript.StdErr.WriteLine(e.description); } >> %_HGET_%
	echo if (res) { WScript.StdErr.WriteLine("OK^!"); WScript.Echo(res); } >> %_HGET_%
	echo else { WScript.StdErr.WriteLine("Failed."); } >> %_HGET_%
goto :EOF

:mk_bndget
	set bnd=%BUNDLE_PATH:\=\\%
	echo /* WSH bundle downloader */ > %_BNDGET_%
	echo var req = new ActiveXObject("MSXML2.XMLHTTP.6.0"); >> %_BNDGET_%
	echo var url = "%BND_URL%"; >> %_BNDGET_%
	echo var res = null; req.Open("GET", url, false); req.Send(); >> %_BNDGET_%
	echo if (req.Status == 200) {  >> %_BNDGET_%
	echo var data=WScript.CreateObject("ADODB.Stream"); >> %_BNDGET_%
	echo data.Open(); data.Type=1;data.Write(req.ResponseBody);data.Position=0; >> %_BNDGET_%
	echo data.SaveToFile("%bnd%", 2);data.Close(); >> %_BNDGET_%
	echo } >> %_BNDGET_%
goto :EOF


:start
set TDM_DIR=%~dp0ext\tools\TDM-GCC
if not x%TDM_HOME%==x (
	set TDM_DIR=%TDM_HOME%
)
echo [Setting up compiler tools...]
if not exist %TDM_DIR%\bin (
	echo ^^! Please install TDM-GCC tools:
	echo ^^! https://jmeubank.github.io/tdm-gcc/download/
	echo ^^! To build:
	echo ^^! cmd /c "set TDM_HOME=^<path^>&&build.bat"
	exit /B -1
)
set TDM_BIN=%TDM_DIR%\bin
set TDM_GCC=%TDM_BIN%\gcc.exe
set TDM_GPP=%TDM_BIN%\g++.exe
set TDM_MAK=%TDM_BIN%\mingw32-make.exe
set TDM_WRS=%TDM_BIN%\windres.exe
echo [C++ compiler path: %TDM_GPP%]

set _CSCR_=cscript.exe /nologo

if not exist %TMP_DIR% mkdir %TMP_DIR%
set _HGET_=%TMP_DIR%\hget.js
set _BNDGET_=%TMP_DIR%\bndget.js

echo [Checking OpenGL headers...]
if not exist %_HGET_% call :mk_hget
if not exist %INC_DIR%\GL mkdir %INC_DIR%\GL
if not exist %INC_DIR%\KHR mkdir %INC_DIR%\KHR
set KHR_REG=https://www.khronos.org/registry
for %%i in (GL\glcorearb.h GL\glext.h GL\wglext.h KHR\khrplatform.h) do (
	if not exist %INC_DIR%\%%i (
		set rel=%%i
		set rel=!rel:GL\=OpenGL/api/GL/!
		set rel=!rel:KHR\=EGL/api/KHR/!
		%_CSCR_% %_HGET_% %KHR_REG%/!rel! >  %INC_DIR%\%%i
	)
)

rem goto :xskip
echo [Checking crosscore sources...]
if not exist %XCORE_DIR% mkdir %XCORE_DIR%
if not exist %XCORE_DIR%\ogl mkdir %XCORE_DIR%\ogl
set XCORE_BASE=https://raw.githubusercontent.com/schaban/crosscore_dev/main/src
for %%i in (crosscore.hpp crosscore.cpp demo.hpp demo.cpp draw.hpp oglsys.hpp oglsys.cpp oglsys.inc scene.hpp scene.cpp smprig.hpp smprig.cpp draw_ogl.cpp  ogl\gpu_defs.h ogl\progs.inc ogl\shaders.inc) do (
	if not exist %XCORE_DIR%\%%i (
		set rel=%%i
		%_CSCR_% %_HGET_% %XCORE_BASE%/!rel:\=/! > %XCORE_DIR%\%%i
	)
)
:xskip

if not exist %BUNDLE_PATH% (
	echo ^^! Cannot find resource bundle^^! 
	echo ^^! Downloading from %BND_URL%
	if not exist %_BNDGET_% call :mk_bndget
	%_CSCR_% %_BNDGET_%
)

if exist %BUNDLE_PATH% (
	echo [Bundle location: %BUNDLE_PATH%]
)

if exist %PROG_PATH% (
	echo [Removing previously compiled executable %PROG_PATH%...]
	del %PROG_PATH%
)

set SRC_FILES=
for /f %%i in ('dir /b %SRC_DIR%\*.cpp') do (
	set SRC_FILES=!SRC_FILES! %SRC_DIR%/%%i
)
for /f %%i in ('dir /b %XCORE_DIR%\*.cpp') do (
	set SRC_FILES=!SRC_FILES! %XCORE_DIR%/%%i
)

set CPP_OFLGS=-ffast-math -ftree-vectorize
rem -O3 -flto
set XCORE_FLAGS=-DOGLSYS_ES=0 -DOGLSYS_CL=0 -DDRW_NO_VULKAN=1 -DXD_TSK_NATIVE=1
set CPP_OPTS=%CPP_OFLGS% -std=c++11 -mavx -mf16c -mfpmath=sse -fno-use-linker-plugin -Wno-psabi -Wno-deprecated-declarations
set LNK_OPTS=-l gdi32 -l ole32 -l windowscodecs
echo [Compiling %PROG_PATH%...]
%TDM_GPP% %CPP_OPTS% %XCORE_FLAGS% -I %INC_DIR% -I %XCORE_DIR% -I %SRC_DIR% %SRC_FILES% -o %PROG_PATH% %LNK_OPTS% %*
if exist %PROG_PATH% (
	echo Success^^!
	%TDM_BIN%\objdump.exe -M intel -d %PROG_PATH% > %PROG_DIR%\%PROG_NAME%.txt
) else (
	echo Failure :(
)
