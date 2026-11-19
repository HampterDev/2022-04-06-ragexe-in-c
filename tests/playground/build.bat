@echo off
REM tests\playground\build.bat -> build\ragexe_playground.exe, the Windows client with the
REM playground driver (tests\playground\README.md): the scene harness's clock, sockets, threads,
REM settings and input, and play_driver.c in the place of the scene driver, with the lossy wire
REM of tools\lossy_link for its line. It links the objects build.bat made, so run build.bat first.
setlocal enabledelayedexpansion
set "ROOT=%~dp0..\.."
set "BUILD=%ROOT%\build"
set "VDIR=%BUILD%\vendor"
set "OBJ=%BUILD%\obj_playground"
set "RSP=%BUILD%\ragexe_playground.objs.rsp"
if not exist "%BUILD%\ragexe.objs.rsp" ( echo Run build.bat first. & exit /b 1 )
if not exist "%OBJ%" mkdir "%OBJ%"

call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set VSCMD_SKIP_SENDTELEMETRY=1

set "SCENES=%ROOT%\tests\scenes"
set "PLAY=%ROOT%\tests\playground"
cl /nologo /std:c11 /c /O2 /Oy- /MD /GS /Gd /we4013 /source-charset:utf-8 /execution-charset:.949 ^
 /I"%ROOT%\src" /I"%SCENES%" /I"%PLAY%" /I"%ROOT%\tools" /I"%ROOT%\vendor\stb" /I"%ROOT%\vendor\lua-5.1.4\src" ^
 "%SCENES%\scene_clock.c" "%SCENES%\scene_net.c" "%SCENES%\scene_thread.c" "%SCENES%\scene_settings.c" ^
 "%SCENES%\scene_input_win.c" "%SCENES%\scene_granny_win.c" "%SCENES%\scene_crash_win.c" ^
 "%PLAY%\play_driver.c" "%PLAY%\play_link.c" "%ROOT%\tools\lossy_link\wire.c" /Fo:"%OBJ%\\"
if errorlevel 1 ( echo *** playground compilation FAILED *** & exit /b 1 )

REM The client's objects without the ones the harness replaces.
type nul > "%RSP%"
for /f "usebackq delims=" %%l in ("%BUILD%\ragexe.objs.rsp") do (
  set "skip="
  for %%n in (net_win32 net_enet clock_win32 thread_win32 drive_state settings_win32 window_win32 crash_dialog) do if /i "%%~nl"=="%%n" set "skip=1"
  if not defined skip >> "%RSP%" echo %%l
)
for %%f in ("%OBJ%\*.obj") do >> "%RSP%" echo "%%~ff"

set "SYSLIBS=kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib oleaut32.lib ws2_32.lib winmm.lib ddraw.lib d3d9.lib dxguid.lib gdiplus.lib imm32.lib ole32.lib uuid.lib netapi32.lib"
set "VLIB=%BUILD%\vendor"
link /nologo msvcrt.lib msvcprt.lib @"%RSP%" ^
 "!VLIB!\imglibs.lib" "!VLIB!\lua514.lib" "!VLIB!\yyjson.lib" "!VLIB!\crijndael.lib" "!VLIB!\font_backend.lib" "!VLIB!\jpeg.lib" /LIBPATH:"%ROOT%\lib" ^
 cps.lib granny2.lib mss32.lib shlwapi.lib ^
 msvcp140.lib vcruntime.lib ucrt.lib legacy_stdio_definitions.lib %SYSLIBS% ^
 /OUT:"%BUILD%\ragexe_playground.exe" /SUBSYSTEM:WINDOWS /MACHINE:X86 /SAFESEH /LARGEADDRESSAWARE /OPT:NOICF
if errorlevel 1 ( echo *** playground LINK FAILED *** & exit /b 1 )
echo Done -^> "%BUILD%\ragexe_playground.exe"
