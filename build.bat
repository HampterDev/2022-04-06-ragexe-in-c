@echo off
REM build.bat          -> build\ragexe.exe      (64-bit release)
REM build.bat dbg      -> build\ragexe_dbg.exe  (optimized, with build\ragexe_dbg.pdb)
REM build.bat check    -> also checks the library table on the objects it built
REM                       (tools\check_libraries.bat) and that the shaders' headers match their
REM                       sources (tools\shaders\check.py); combines with the other words
REM build.bat libs     -> build\ragexe_libs.exe: the same objects, archived per library of
REM                       tools\libraries.txt (build\obj_libs\*.lib, next to the variant's object
REM                       folder) and linked as app's objects plus those libraries; combines with
REM                       the other words
REM The client draws Granny models from converted glTF files (tools\granny_export), plays sound
REM through SDL3 (platform\audio_sdl.c), draws text with FreeType and talks to the server over
REM ENet (platform\net_enet.c). It links the C runtime
REM statically (/MT), as do the vendor libraries, so it needs no Visual C++ runtime DLL: the 32-bit
REM ones game folders ship serve only 32-bit programs.
setlocal enabledelayedexpansion
set "ROOT=%~dp0"
cd /d "%ROOT%"
set "BUILD=%ROOT%build"
if not exist "%BUILD%" mkdir "%BUILD%"

set "WANTDBG="
set "CHECK="
set "LIBS="
for %%a in (%*) do (
  if /i "%%~a"=="dbg" set "WANTDBG=1"
  if /i "%%~a"=="check" set "CHECK=1"
  if /i "%%~a"=="libs" set "LIBS=1"
)
set "VDIR=%BUILD%\vendor"
if defined WANTDBG (
  set "OUT=ragexe_dbg"
  set "OBJ=%BUILD%\obj_dbg"
  set "LINKDBG=/DEBUG /INCREMENTAL:NO /PDBALTPATH:ragexe_dbg.pdb"
  set "CDBG=/Z7"
) else (
  set "OUT=ragexe"
  set "OBJ=%BUILD%\obj"
  set "LINKDBG="
  set "CDBG="
)
if not exist "%OBJ%" mkdir "%OBJ%"
if defined LIBS set "OUT=%OUT:ragexe=ragexe_libs%"
if defined LIBS if defined WANTDBG set "LINKDBG=/DEBUG /INCREMENTAL:NO /PDBALTPATH:%OUT%.pdb"

call "%ROOT%tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set VSCMD_SKIP_SENDTELEMETRY=1

echo [1/3] Dependencies...
call "%ROOT%vendor\build_vendor.bat"
if errorlevel 1 ( echo *** VENDOR BUILD FAILED *** & exit /b 1 )
REM The pieces of the set game the client shares with the server, rules (which links script-vm)
REM and delivery, as their own CMakeLists.txt builds them, through the client's CMake tree.
set "PIECES=%BUILD%\pieces"
if not exist "%PIECES%\build.ninja" (
  cmake -S "%ROOT%." -B "%PIECES%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded >nul
  if errorlevel 1 ( echo *** PIECES CONFIGURE FAILED: is the submodule game checked out? *** & exit /b 1 )
)
cmake --build "%PIECES%" --target rules delivery script_vm
if errorlevel 1 ( echo *** PIECES BUILD FAILED *** & exit /b 1 )

echo [2/3] Compiling...
REM data\effects\effects.json is compiled in as a byte array (tools\bin2c.c), so the
REM executable does not depend on the file being shipped.
set "GEN=%BUILD%\gen"
if not exist "%GEN%" mkdir "%GEN%"
cl /nologo /O2 /MD "%ROOT%tools\bin2c.c" /Fo:"%GEN%\\" /Fe:"%GEN%\bin2c.exe"
if errorlevel 1 ( echo *** bin2c FAILED *** & exit /b 1 )
"%GEN%\bin2c.exe" "%GEN%\effect_data_embedded.c" g_effectData "%ROOT%data\effects\effects.json"
if errorlevel 1 ( echo *** EMBEDDING data\effects FAILED *** & exit /b 1 )
type nul > "%BUILD%\sources.rsp"
type nul > "%BUILD%\%OUT%.objs.rsp"
for /r "%ROOT%src" %%f in (*.c) do (
  >> "%BUILD%\sources.rsp" echo "%%f"
  >> "%BUILD%\%OUT%.objs.rsp" echo "%OBJ%\%%~nf.obj"
)
>> "%BUILD%\sources.rsp" echo "%GEN%\effect_data_embedded.c"
>> "%BUILD%\%OUT%.objs.rsp" echo "%OBJ%\effect_data_embedded.obj"
set "INC=/I"%ROOT%src" /I"%ROOT%game\rules" /I"%ROOT%game\delivery" /I"%ROOT%vendor\libpng-1.6.36" /I"%ROOT%vendor\zlib-1.2.11" /I"%ROOT%vendor\lua-5.1.4\src" /I"%ROOT%vendor\yyjson-0.13.0" /I"%ROOT%vendor\stb" /I"%ROOT%vendor\cgltf" /I"%ROOT%vendor\dr_libs" /I"%ROOT%vendor\enet-1.3.18\include" /I"%VDIR%\sdl3-src\include" /I"%VDIR%\freetype-src\include" /DDR_MP3_NO_STDIO"
cl /nologo /std:c11 /c /MP4 /O2 /Oy- /MT /GS /Gd /we4013 /source-charset:utf-8 /execution-charset:.949 %INC% %CDBG% @"%BUILD%\sources.rsp" /Fo:"%OBJ%\\"
if errorlevel 1 ( echo. & echo *** C compilation FAILED *** & exit /b 1 )
if defined LIBS (
  echo [libs] Archiving the libraries...
  set "LIBDIR=%OBJ%_libs"
  call "%ROOT%tools\depmap\symbols.bat" "%OBJ%" "!LIBDIR!.sym"
  if errorlevel 1 exit /b 1
  python "%ROOT%tools\depmap\lib_objects.py" "%OBJ%" "!LIBDIR!.sym" "!LIBDIR!"
  if errorlevel 1 exit /b 1
  copy /y "!LIBDIR!\app.rsp" "%BUILD%\%OUT%.objs.rsp" >nul
  for /f "usebackq" %%l in ("!LIBDIR!\libraries.txt") do (
    lib /nologo /OUT:"!LIBDIR!\%%l.lib" @"!LIBDIR!\%%l.rsp"
    if errorlevel 1 ( echo *** lib %%l FAILED *** & exit /b 1 )
    >> "%BUILD%\%OUT%.objs.rsp" echo "!LIBDIR!\%%l.lib"
  )
)
>> "%BUILD%\%OUT%.objs.rsp" echo "%ROOT%res\app.res"
>> "%BUILD%\%OUT%.objs.rsp" echo "%PIECES%\game\rules\rules.lib"
>> "%BUILD%\%OUT%.objs.rsp" echo "%PIECES%\game\delivery\delivery.lib"
>> "%BUILD%\%OUT%.objs.rsp" echo "%PIECES%\game\script-vm\script_vm.lib"

echo [3/3] Linking...
set "SYSLIBS=kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ws2_32.lib winmm.lib imm32.lib ole32.lib uuid.lib"
link /nologo libcmt.lib @"%BUILD%\%OUT%.objs.rsp" ^
 "%VDIR%\imglibs.lib" "%VDIR%\lua514.lib" "%VDIR%\yyjson.lib" "%VDIR%\crijndael.lib" "%VDIR%\jpeg.lib" "%VDIR%\enet.lib" ^
 "%VDIR%\dr_mp3.lib" "%VDIR%\SDL3-static.lib" setupapi.lib version.lib shlwapi.lib ^
 "%VDIR%\freetype.lib" libvcruntime.lib libucrt.lib %SYSLIBS% ^
 /OUT:"%BUILD%\%OUT%.exe" /IMPLIB:"%BUILD%\%OUT%.lib" /SUBSYSTEM:WINDOWS /MACHINE:X64 ^
 /OPT:NOICF /MAP:"%BUILD%\%OUT%.map" /Brepro %LINKDBG%
if errorlevel 1 ( echo *** LINK FAILED *** & exit /b 1 )
echo Done -^> "%BUILD%\%OUT%.exe"
if defined CHECK (
  call "%ROOT%tools\check_libraries.bat" "%OBJ%"
  if errorlevel 1 ( echo *** LIBRARY CHECK FAILED *** & exit /b 1 )
  python "%ROOT%tools\shaders\check.py"
  if errorlevel 1 ( echo *** SHADER CHECK FAILED *** & exit /b 1 )
)
