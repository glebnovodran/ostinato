@echo off

if x%PROCESSOR_ARCHITECTURE%==xAMD64 (
	set FWK_DIR=Framework64
) else (
	set FWK_DIR=Framework
)

set FWK_PATH="%SystemRoot%\Microsoft.NET\%FWK_DIR%"
set DIR_CMD=dir /B /S /A:-D

set LST_VBC=%DIR_CMD% "%FWK_PATH%\*vbc.exe"
set LST_JSC=%DIR_CMD% "%FWK_PATH%\*jsc.exe"
set LST_CSC=%DIR_CMD% "%FWK_PATH%\*csc.exe"


set _VBC_=
for /f %%i in ('%LST_VBC%') do (set "_VBC_=%%i")
set _JSC_=
for /f %%i in ('%LST_JSC%') do (set "_JSC_=%%i")
set _CSC_=
for /f %%i in ('%LST_CSC%') do (set "_CSC_=%%i")

