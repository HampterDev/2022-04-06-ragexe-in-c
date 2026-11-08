@echo off
REM tests\scenes\build.bat [x64] [freetype] -> build\ragexe_scenes.exe (ragexe_scenes_x64.exe,
REM ragexe_scenes_ft.exe), the Windows client with the scene harness (tests\scenes\README.md). It
REM links the objects build.bat made, so run build.bat with the same arguments first.
setlocal enabledelayedexpansion
set "ROOT=%~dp0..\.."
set "BUILD=%ROOT%\build"
set "ARCH=x86"
set "TEXT="
for %%a in (%*) do (
  if /i "%%~a"=="x64" set "ARCH=x64"
  if /i "%%~a"=="freetype" set "TEXT=freetype"
)
set "SUFFIX="
if "%ARCH%"=="x64" set "SUFFIX=_x64"
if defined TEXT set "SUFFIX=%SUFFIX%_ft"
set "VDIR=%BUILD%\vendor"
if "%ARCH%"=="x64" set "VDIR=%BUILD%\vendor_x64"
set "TEXTLIB="
set "TEXTDEF="
if defined TEXT (
  set "TEXTLIB="%VDIR%\freetype.lib""
  set "TEXTDEF=/DRAGE_TEXT_FREETYPE"
)
set "OBJ=%BUILD%\obj_scenes%SUFFIX%"
set "RSP=%BUILD%\ragexe_scenes%SUFFIX%.objs.rsp"
if not exist "%BUILD%\main_rebuild%SUFFIX%.objs.rsp" ( echo Run build.bat %ARCH% first. & exit /b 1 )
if not exist "%OBJ%" mkdir "%OBJ%"

call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
if "%ARCH%"=="x64" set "VCVARS=%VCVARS:vcvars32.bat=vcvars64.bat%"
call "%VCVARS%" >nul
set VSCMD_SKIP_SENDTELEMETRY=1

set "SCENES=%ROOT%\tests\scenes"
cl /nologo /std:c11 /c /O2 /Oy- /MD /GS /Gd /we4013 /source-charset:utf-8 /execution-charset:.949 %TEXTDEF% ^
 /I"%ROOT%\src" /I"%SCENES%" /I"%ROOT%\vendor\stb" /I"%ROOT%\vendor\lua-5.1.4\src" ^
 "%SCENES%\scene_clock.c" "%SCENES%\scene_net.c" "%SCENES%\scene_thread.c" "%SCENES%\scene_settings.c" ^
 "%SCENES%\scene_input_win.c" "%SCENES%\scene_granny_win.c" "%SCENES%\scene_crash_win.c" "%SCENES%\scene_driver.c" /Fo:"%OBJ%\\"
if errorlevel 1 ( echo *** scene harness compilation FAILED *** & exit /b 1 )

REM The client's objects without the ones the harness replaces.
type nul > "%RSP%"
for /f "usebackq delims=" %%l in ("%BUILD%\main_rebuild%SUFFIX%.objs.rsp") do (
  set "skip="
  for %%n in (net_win32 clock_win32 thread_win32 drive_state settings_win32 window_win32 crash_dialog) do if /i "%%~nl"=="%%n" set "skip=1"
  if not defined skip >> "%RSP%" echo %%l
)
for %%f in ("%OBJ%\*.obj") do >> "%RSP%" echo "%%~ff"

set "SYSLIBS=kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib oleaut32.lib ws2_32.lib winmm.lib ddraw.lib d3d9.lib dxguid.lib gdiplus.lib imm32.lib ole32.lib uuid.lib netapi32.lib"
if "%ARCH%"=="x64" (
  set "VLIB=%BUILD%\vendor_x64"
  link /nologo msvcrt.lib msvcprt.lib @"%RSP%" ^
   "!VLIB!\imglibs.lib" "!VLIB!\lua514.lib" "!VLIB!\yyjson.lib" "!VLIB!\crijndael.lib" "!VLIB!\font_backend.lib" "!VLIB!\jpeg.lib" ^
   "!VLIB!\dr_mp3.lib" "!VLIB!\SDL3-static.lib" setupapi.lib version.lib cfgmgr32.lib shlwapi.lib ^
   !TEXTLIB! vcruntime.lib ucrt.lib legacy_stdio_definitions.lib %SYSLIBS% ^
   /OUT:"%BUILD%\ragexe_scenes%SUFFIX%.exe" /SUBSYSTEM:WINDOWS /MACHINE:X64 /OPT:NOICF
) else (
  set "VLIB=%BUILD%\vendor"
  link /nologo msvcrt.lib msvcprt.lib @"%RSP%" ^
   "!VLIB!\imglibs.lib" "!VLIB!\lua514.lib" "!VLIB!\yyjson.lib" "!VLIB!\crijndael.lib" "!VLIB!\font_backend.lib" "!VLIB!\jpeg.lib" /LIBPATH:"%ROOT%\lib" ^
   cps.lib granny2.lib mss32.lib shlwapi.lib ^
   !TEXTLIB! msvcp140.lib vcruntime.lib ucrt.lib legacy_stdio_definitions.lib %SYSLIBS% ^
   /OUT:"%BUILD%\ragexe_scenes%SUFFIX%.exe" /SUBSYSTEM:WINDOWS /MACHINE:X86 /SAFESEH /LARGEADDRESSAWARE /OPT:NOICF
)
if errorlevel 1 ( echo *** scene harness LINK FAILED *** & exit /b 1 )
echo Done -^> "%BUILD%\ragexe_scenes%SUFFIX%.exe"
