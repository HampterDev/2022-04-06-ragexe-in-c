@echo off
REM The third-party libraries, from the sources in this folder, to ..\build\vendor\*.lib.
setlocal
pushd "%~dp0" || exit /b 1
set "OUT=%~dp0..\build\vendor"
if not exist "%OUT%" mkdir "%OUT%"
set "CFLAGS=/nologo /c /O2 /MD /MP4 /DNDEBUG /D_CRT_SECURE_NO_WARNINGS"

call :library imglibs "/Izlib-1.2.11" zlib-1.2.11\*.c libpng-1.6.36\*.c giflib\*.c
if errorlevel 1 goto failed
call :library lua514 "" lua-5.1.4\src\*.c
if errorlevel 1 goto failed
call :library yyjson "/DYYJSON_READER_DEPTH_LIMIT=256 /DYYJSON_WRITER_DEPTH_LIMIT=256" yyjson-0.13.0\*.c
if errorlevel 1 goto failed
call :library crijndael "/EHsc /GR- /I..\src" crijndael\*.cpp
if errorlevel 1 goto failed
call :library font_backend "/EHsc /GR- /I..\src" font_backend\*.cpp
if errorlevel 1 goto failed
if errorlevel 1 goto failed
popd
exit /b 0

:failed
popd
exit /b 1

:library
set "NAME=%~1"
set "EXTRA=%~2"
if not exist "%OUT%\obj\%NAME%" mkdir "%OUT%\obj\%NAME%"
del /q "%OUT%\obj\%NAME%\*.obj" 2>nul
cl %CFLAGS% %EXTRA% %3 %4 %5 /Fo"%OUT%\obj\%NAME%\\"
if errorlevel 1 exit /b 1
lib /nologo /OUT:"%OUT%\%NAME%.lib" "%OUT%\obj\%NAME%\*.obj"
exit /b %errorlevel%
