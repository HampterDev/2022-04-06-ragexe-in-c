@echo off
REM The third-party libraries, from the sources in this folder, to ..\build\vendor\*.lib, 64-bit
REM and with the static C runtime (/MT) like the client: zlib and libpng, Lua, yyjson, stb's JPEG,
REM ENet (platform\net_enet.c's link), the client's C++ Rijndael, dr_mp3, SDL3 (platform\audio_sdl.c plays sound through them) and
REM FreeType, which draws the text.
REM Run from a vcvars64 environment (tools\find_vcvars.bat).
setlocal
pushd "%~dp0" || exit /b 1
set "OUT=%~dp0..\build\vendor"

REM build\vendor\kind.txt says what the libraries there were built as; it is written once all but
REM FreeType are built. A folder without it, or built as anything else (32-bit until 2026-11-11,
REM then with the DLL runtime), is removed and built again from clean, since the linker would
REM otherwise take its libraries. Change KIND whenever the libraries must be built again.
set "KIND=x64 /MT, SDL without OpenGL"
set "STAMP=%OUT%\kind.txt"
set "BUILT="
if exist "%STAMP%" set /p BUILT=<"%STAMP%"
if not "%BUILT%"=="%KIND%" if exist "%OUT%" (
  echo build\vendor was not built as "%KIND%": building it again from clean.
  rd /s /q "%OUT%"
  if exist "%OUT%" ( echo *** Cannot remove "%OUT%" & goto failed )
)
if not exist "%OUT%" mkdir "%OUT%"
REM Not CFLAGS, which CMake would take as the C flags of SDL3 and FreeType.
set "CLOPTS=/nologo /c /O2 /MT /MP4 /DNDEBUG /D_CRT_SECURE_NO_WARNINGS"

call :library imglibs "/Izlib-1.2.11" zlib-1.2.11\*.c libpng-1.6.36\*.c
if errorlevel 1 goto failed
call :library lua514 "" lua-5.1.4\src\*.c
if errorlevel 1 goto failed
call :library yyjson "/DYYJSON_READER_DEPTH_LIMIT=256 /DYYJSON_WRITER_DEPTH_LIMIT=256" yyjson-0.13.0\*.c
if errorlevel 1 goto failed
call :library jpeg "" stb\stb_impl.c
if errorlevel 1 goto failed
call :library enet "/Ienet-1.3.18\include" enet-1.3.18\*.c
if errorlevel 1 goto failed
call :library crijndael "/EHsc /GR- /I..\src" crijndael\*.cpp
if errorlevel 1 goto failed
call :library dr_mp3 "/DDR_MP3_NO_STDIO" dr_libs\dr_mp3_impl.c
if errorlevel 1 goto failed
call :sdl3
if errorlevel 1 goto failed
>"%STAMP%" echo %KIND%
call :freetype
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
cl %CLOPTS% %EXTRA% %3 %4 %5 /Fo"%OUT%\obj\%NAME%\\"
if errorlevel 1 exit /b 1
lib /nologo /OUT:"%OUT%\%NAME%.lib" "%OUT%\obj\%NAME%\*.obj"
exit /b %errorlevel%

REM SDL3 at the commit cmake\SDL3.cmake pins, static, with its audio, video and GPU subsystems: the
REM client plays sound through it and runs in its window, and the renderer of plans\gpu-renderer.md
REM will use its GPU API on Vulkan. OpenGL and OpenGL ES (with EGL) are left out: nothing will use them. SDL has
REM no switch for its Direct3D 12 GPU driver, which it builds whenever dxgi1_6.h is there (the same
REM check gives its Windows video driver the displays' HDR properties), so that driver stays. SDL's
REM dynamic API table refers to every function it has, so the client links the GPU code too, which
REM it does not call yet; none of it needs a system library beyond those build.bat links.
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
 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded ^
 -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF ^
 -DSDL_VIDEO=ON -DSDL_GPU=ON -DSDL_VULKAN=ON -DSDL_OPENGL=OFF -DSDL_OPENGLES=OFF ^
 -DSDL_RENDER=OFF -DSDL_CAMERA=OFF -DSDL_JOYSTICK=OFF -DSDL_HAPTIC=OFF ^
 -DSDL_HIDAPI=OFF -DSDL_SENSOR=OFF -DSDL_POWER=OFF -DSDL_DIALOG=OFF -DSDL_TRAY=OFF >"%OUT%\sdl3-configure.log" 2>&1
if errorlevel 1 ( type "%OUT%\sdl3-configure.log" & exit /b 1 )
cmake --build "%OUT%\sdl3-build" >"%OUT%\sdl3-build.log" 2>&1
if errorlevel 1 ( type "%OUT%\sdl3-build.log" & exit /b 1 )
copy /y "%OUT%\sdl3-build\SDL3-static.lib" "%OUT%\SDL3-static.lib" >nul
exit /b %errorlevel%

REM FreeType at the release cmake\Freetype.cmake fetches for Android, static, with only what the
REM client reads (no compressed fonts, no PNG glyphs, no shaping). Its CMake files predate the
REM runtime setting (policy CMP0091), so the policy is switched on for it.
:freetype
if exist "%OUT%\freetype.lib" exit /b 0
set "FT_TAG=VER-2-13-3"
set "FT_SHA256=BC5C898E4756D373E0D991BAB053036C5EB2AA7C0D5C67E8662DDC6DA40C4103"
set "FT_TAR=%OUT%\freetype.tar.gz"
if not exist "%OUT%\freetype-src\CMakeLists.txt" (
  curl -sSfL -o "%FT_TAR%" "https://codeload.github.com/freetype/freetype/tar.gz/refs/tags/%FT_TAG%" || exit /b 1
  powershell -NoProfile -Command "if ((Get-FileHash -Algorithm SHA256 '%FT_TAR%').Hash -ne '%FT_SHA256%') { exit 1 }" || ( echo *** FreeType download does not match its hash & exit /b 1 )
  if not exist "%OUT%\freetype-src" mkdir "%OUT%\freetype-src"
  tar -xzf "%FT_TAR%" -C "%OUT%\freetype-src" --strip-components=1 || exit /b 1
)
set "GEN=NMake Makefiles"
where ninja >nul 2>nul && set "GEN=Ninja"
cmake -S "%OUT%\freetype-src" -B "%OUT%\freetype-build" -G "%GEN%" -DCMAKE_BUILD_TYPE=Release ^
 -DCMAKE_POLICY_DEFAULT_CMP0091=NEW -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded ^
 -DBUILD_SHARED_LIBS=OFF -DFT_DISABLE_ZLIB=ON -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_PNG=ON ^
 -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON >"%OUT%\freetype-configure.log" 2>&1
if errorlevel 1 ( type "%OUT%\freetype-configure.log" & exit /b 1 )
cmake --build "%OUT%\freetype-build" >"%OUT%\freetype-build.log" 2>&1
if errorlevel 1 ( type "%OUT%\freetype-build.log" & exit /b 1 )
copy /y "%OUT%\freetype-build\freetype.lib" "%OUT%\freetype.lib" >nul
exit /b %errorlevel%
