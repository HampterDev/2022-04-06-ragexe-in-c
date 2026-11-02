@echo off
REM build.bat          -> build\main_rebuild.exe      (32-bit release)
REM build.bat dbg      -> build\main_dbg.exe          (optimized, with build\main_dbg.pdb)
REM build.bat x64      -> build\main_rebuild_x64.exe  (64-bit; build.bat x64 dbg -> main_dbg_x64.exe)
REM build.bat freetype -> build\main_rebuild_ft.exe   (text through FreeType instead of GDI, as off
REM                       Windows; with x64 -> main_rebuild_x64_ft.exe)
REM build.bat check    -> also checks the library table on the objects it built
REM                       (tools\check_libraries.bat; combines with the other words)
REM build.bat libs     -> build\main_libs_rebuild.exe: the same objects, archived per library of
REM                       tools\libraries.txt (build\obj_libs\*.lib, next to the variant's object
REM                       folder) and linked as app's objects plus those libraries; combines with
REM                       the other words
REM The 32-bit client links RAD's granny2.dll and mss32.dll; the 64-bit one draws Granny models
REM from converted glTF files (tools\granny_export) and plays sound through SDL3.
setlocal enabledelayedexpansion
set "ROOT=%~dp0"
cd /d "%ROOT%"
set "BUILD=%ROOT%build"
if not exist "%BUILD%" mkdir "%BUILD%"

set "WANTDBG="
set "ARCH=x86"
set "TEXT="
set "CHECK="
set "LIBS="
for %%a in (%*) do (
  if /i "%%~a"=="dbg" set "WANTDBG=1"
  if /i "%%~a"=="x64" set "ARCH=x64"
  if /i "%%~a"=="freetype" set "TEXT=freetype"
  if /i "%%~a"=="check" set "CHECK=1"
  if /i "%%~a"=="libs" set "LIBS=1"
)
set "SUFFIX="
if "%ARCH%"=="x64" set "SUFFIX=_x64"
if defined TEXT set "SUFFIX=%SUFFIX%_ft"
set "VDIR=%BUILD%\vendor"
if "%ARCH%"=="x64" set "VDIR=%BUILD%\vendor_x64"
if defined WANTDBG (
  set "OUT=main_dbg%SUFFIX%"
  set "OBJ=%BUILD%\obj_dbg%SUFFIX%"
  set "LINKDBG=/DEBUG /INCREMENTAL:NO /PDBALTPATH:main_dbg%SUFFIX%.pdb"
  set "CDBG=/Z7"
) else (
  set "OUT=main_rebuild%SUFFIX%"
  set "OBJ=%BUILD%\obj%SUFFIX%"
  set "LINKDBG="
  set "CDBG="
)
if not exist "%OBJ%" mkdir "%OBJ%"
if defined LIBS set "OUT=%OUT:main_=main_libs_%"
if defined LIBS if defined WANTDBG set "LINKDBG=/DEBUG /INCREMENTAL:NO /PDBALTPATH:%OUT%.pdb"

call "%ROOT%tools\find_vcvars.bat" || exit /b 1
if "%ARCH%"=="x64" set "VCVARS=%VCVARS:vcvars32.bat=vcvars64.bat%"
call "%VCVARS%" >nul
set VSCMD_SKIP_SENDTELEMETRY=1

echo [1/3] Dependencies...
call "%ROOT%vendor\build_vendor.bat" %ARCH% %TEXT%
if errorlevel 1 ( echo *** VENDOR BUILD FAILED *** & exit /b 1 )

echo [2/3] Compiling...
REM data\effects\effects.json is compiled in as a byte array (tools\bin2c.c), so the
REM executable does not depend on the file being shipped.
set "GEN=%BUILD%\gen"
if not exist "%GEN%" mkdir "%GEN%"
cl /nologo /O2 /MD "%ROOT%tools\bin2c.c" /Fo:"%GEN%\\" /Fe:"%GEN%\bin2c.exe"
if errorlevel 1 ( echo *** bin2c FAILED *** & exit /b 1 )
"%GEN%\bin2c.exe" "%GEN%\effect_data_embedded.c" g_effectData "%ROOT%data\effects\effects.json"
if errorlevel 1 ( echo *** EMBEDDING data\effects FAILED *** & exit /b 1 )
type nul > "%BUILD%\sources%SUFFIX%.rsp"
type nul > "%BUILD%\%OUT%.objs.rsp"
for /r "%ROOT%src" %%f in (*.c) do (
  >> "%BUILD%\sources%SUFFIX%.rsp" echo "%%f"
  >> "%BUILD%\%OUT%.objs.rsp" echo "%OBJ%\%%~nf.obj"
)
>> "%BUILD%\sources%SUFFIX%.rsp" echo "%GEN%\effect_data_embedded.c"
>> "%BUILD%\%OUT%.objs.rsp" echo "%OBJ%\effect_data_embedded.obj"
set "INC=/I"%ROOT%src" /I"%ROOT%vendor\libpng-1.6.36" /I"%ROOT%vendor\zlib-1.2.11" /I"%ROOT%vendor\lua-5.1.4\src" /I"%ROOT%vendor\yyjson-0.13.0""
if "%ARCH%"=="x64" set "INC=%INC% /I"%ROOT%vendor\stb" /I"%ROOT%vendor\cgltf" /I"%ROOT%vendor\dr_libs" /I"%BUILD%\vendor_x64\sdl3-src\include" /DDR_MP3_NO_STDIO"
set "TEXTLIB="
if defined TEXT (
  set "INC=%INC% /DRAGE_TEXT_FREETYPE /I"%VDIR%\freetype-src\include""
  set "TEXTLIB="%VDIR%\freetype.lib""
)
cl /nologo /std:c11 /c /MP4 /O2 /Oy- /MD /GS /Gd /we4013 /source-charset:utf-8 /execution-charset:.949 %INC% %CDBG% @"%BUILD%\sources%SUFFIX%.rsp" /Fo:"%OBJ%\\"
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

echo [3/3] Linking...
set "SYSLIBS=kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib oleaut32.lib ws2_32.lib winmm.lib ddraw.lib d3d9.lib dxguid.lib gdiplus.lib imm32.lib ole32.lib uuid.lib netapi32.lib"
if "%ARCH%"=="x64" (
  set "VLIB=%BUILD%\vendor_x64"
  link /nologo msvcrt.lib msvcprt.lib @"%BUILD%\%OUT%.objs.rsp" ^
   "!VLIB!\imglibs.lib" "!VLIB!\lua514.lib" "!VLIB!\yyjson.lib" "!VLIB!\crijndael.lib" "!VLIB!\font_backend.lib" "!VLIB!\jpeg.lib" ^
   "!VLIB!\dr_mp3.lib" "!VLIB!\SDL3-static.lib" setupapi.lib version.lib cfgmgr32.lib shlwapi.lib ^
   !TEXTLIB! vcruntime.lib ucrt.lib legacy_stdio_definitions.lib %SYSLIBS% ^
   /OUT:"%BUILD%\%OUT%.exe" /IMPLIB:"%BUILD%\%OUT%.lib" /SUBSYSTEM:WINDOWS /MACHINE:X64 ^
   /OPT:NOICF /MAP:"%BUILD%\%OUT%.map" /Brepro %LINKDBG%
) else (
  set "VLIB=%BUILD%\vendor"
  link /nologo msvcrt.lib msvcprt.lib @"%BUILD%\%OUT%.objs.rsp" ^
   "!VLIB!\imglibs.lib" "!VLIB!\lua514.lib" "!VLIB!\yyjson.lib" "!VLIB!\crijndael.lib" "!VLIB!\font_backend.lib" "!VLIB!\jpeg.lib" /LIBPATH:"%ROOT%lib" ^
   cps.lib granny2.lib mss32.lib shlwapi.lib ^
   !TEXTLIB! msvcp140.lib vcruntime.lib ucrt.lib legacy_stdio_definitions.lib %SYSLIBS% ^
   /OUT:"%BUILD%\%OUT%.exe" /IMPLIB:"%BUILD%\%OUT%.lib" /SUBSYSTEM:WINDOWS /MACHINE:X86 /SAFESEH /LARGEADDRESSAWARE ^
   /OPT:NOICF /MAP:"%BUILD%\%OUT%.map" /Brepro %LINKDBG%
)
if errorlevel 1 ( echo *** LINK FAILED *** & exit /b 1 )
echo Done -^> "%BUILD%\%OUT%.exe"
if defined CHECK (
  call "%ROOT%tools\check_libraries.bat" "%OBJ%"
  if errorlevel 1 ( echo *** LIBRARY CHECK FAILED *** & exit /b 1 )
)
