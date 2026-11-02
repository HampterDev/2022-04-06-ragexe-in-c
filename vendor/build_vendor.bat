@echo off
REM The third-party libraries, from the sources in this folder, to ..\build\vendor\*.lib
REM (build_vendor.bat x64: ..\build\vendor_x64\*.lib, plus dr_mp3 and an audio-only SDL3 for the
REM Miles replacement, platform\miles_sdl.c). Run from a vcvars environment of the same width.
setlocal
pushd "%~dp0" || exit /b 1
set "ARCH=x86"
if /i "%~1"=="x64" set "ARCH=x64"
if "%ARCH%"=="x64" (set "OUT=%~dp0..\build\vendor_x64") else (set "OUT=%~dp0..\build\vendor")
if not exist "%OUT%" mkdir "%OUT%"
set "CFLAGS=/nologo /c /O2 /MD /MP4 /DNDEBUG /D_CRT_SECURE_NO_WARNINGS"

call :library imglibs "/Izlib-1.2.11" zlib-1.2.11\*.c libpng-1.6.36\*.c giflib\*.c
if errorlevel 1 goto failed
call :library lua514 "" lua-5.1.4\src\*.c
if errorlevel 1 goto failed
call :library yyjson "/DYYJSON_READER_DEPTH_LIMIT=256 /DYYJSON_WRITER_DEPTH_LIMIT=256" yyjson-0.13.0\*.c
if errorlevel 1 goto failed
call :library jpeg "" stb\stb_impl.c
if errorlevel 1 goto failed
call :library crijndael "/EHsc /GR- /I..\src" crijndael\*.cpp
if errorlevel 1 goto failed
call :library font_backend "/EHsc /GR- /I..\src" font_backend\*.cpp
if errorlevel 1 goto failed
if "%ARCH%"=="x64" (
  call :library dr_mp3 "/DDR_MP3_NO_STDIO" dr_libs\dr_mp3_impl.c
  if errorlevel 1 goto failed
  call :sdl3
  if errorlevel 1 goto failed
)
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

REM SDL3 at the commit cmake\SDL3.cmake pins, static, with only its audio subsystem.
:sdl3
if exist "%OUT%\SDL3-static.lib" exit /b 0
set "SDL_COMMIT=829a65d769d935c4852f8159e964312c0957260a"
set "SDL_SHA256=48e15cf1a6928453d77afa955d1fdfbbca9169e2bc3d76d111acb9b8bb076d66"
set "SDL_TAR=%OUT%\sdl3.tar.gz"
if not exist "%OUT%\sdl3-src\CMakeLists.txt" (
  curl -sSfL -o "%SDL_TAR%" "https://codeload.github.com/libsdl-org/SDL/tar.gz/%SDL_COMMIT%" || exit /b 1
  powershell -NoProfile -Command "if ((Get-FileHash -Algorithm SHA256 '%SDL_TAR%').Hash -ne '%SDL_SHA256%') { exit 1 }" || ( echo *** SDL3 download does not match its hash & exit /b 1 )
  if not exist "%OUT%\sdl3-src" mkdir "%OUT%\sdl3-src"
  tar -xzf "%SDL_TAR%" -C "%OUT%\sdl3-src" --strip-components=1 || exit /b 1
)
set "GEN=NMake Makefiles"
where ninja >nul 2>nul && set "GEN=Ninja"
cmake -S "%OUT%\sdl3-src" -B "%OUT%\sdl3-build" -G "%GEN%" -DCMAKE_BUILD_TYPE=Release ^
 -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF ^
 -DSDL_VIDEO=OFF -DSDL_RENDER=OFF -DSDL_GPU=OFF -DSDL_CAMERA=OFF -DSDL_JOYSTICK=OFF -DSDL_HAPTIC=OFF ^
 -DSDL_HIDAPI=OFF -DSDL_SENSOR=OFF -DSDL_POWER=OFF -DSDL_DIALOG=OFF -DSDL_TRAY=OFF >"%OUT%\sdl3-configure.log" 2>&1
if errorlevel 1 ( type "%OUT%\sdl3-configure.log" & exit /b 1 )
cmake --build "%OUT%\sdl3-build" >"%OUT%\sdl3-build.log" 2>&1
if errorlevel 1 ( type "%OUT%\sdl3-build.log" & exit /b 1 )
copy /y "%OUT%\sdl3-build\SDL3-static.lib" "%OUT%\SDL3-static.lib" >nul
exit /b %errorlevel%
