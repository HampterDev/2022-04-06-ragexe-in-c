@echo off
REM tests\scenes\build.bat -> build\ragexe_scenes.exe, the Windows client with the scene harness
REM (tests\scenes\README.md). It links the objects build.bat made, so run build.bat first.
setlocal enabledelayedexpansion
set "ROOT=%~dp0..\.."
set "BUILD=%ROOT%\build"
set "VDIR=%BUILD%\vendor"
set "OBJ=%BUILD%\obj_scenes"
set "RSP=%BUILD%\ragexe_scenes.objs.rsp"
if not exist "%BUILD%\ragexe.objs.rsp" ( echo Run build.bat first. & exit /b 1 )
if not exist "%OBJ%" mkdir "%OBJ%"
REM Every object in OBJ is linked, so none may be left from a harness file that has gone.
del /q "%OBJ%\*.obj" 2>nul

call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set VSCMD_SKIP_SENDTELEMETRY=1

set "SCENES=%ROOT%\tests\scenes"
cl /nologo /std:c11 /c /O2 /Oy- /MT /GS /Gd /we4013 /source-charset:utf-8 /execution-charset:.949 ^
 /I"%ROOT%\src" /I"%ROOT%\game\rules" /I"%ROOT%\game\delivery" /I"%SCENES%" /I"%ROOT%\vendor\stb" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%VDIR%\sdl3-src\include" ^
 "%SCENES%\scene_clock.c" "%SCENES%\scene_net.c" "%SCENES%\scene_thread.c" "%SCENES%\scene_settings.c" ^
 "%SCENES%\scene_app_hidden.c" "%SCENES%\scene_window_win.c" "%SCENES%\scene_crash_win.c" "%SCENES%\scene_driver.c" ^
 "%SCENES%\scene_world.c" /Fo:"%OBJ%\\"
if errorlevel 1 ( echo *** scene harness compilation FAILED *** & exit /b 1 )

REM The client's objects without the ones the harness replaces.
type nul > "%RSP%"
for /f "usebackq delims=" %%l in ("%BUILD%\ragexe.objs.rsp") do (
  set "skip="
  for %%n in (net_win32 net_enet clock_win32 thread_win32 drive_state settings_win32 app_sdl window_sdl crash_dialog) do if /i "%%~nl"=="%%n" set "skip=1"
  if not defined skip >> "%RSP%" echo %%l
)
for %%f in ("%OBJ%\*.obj") do >> "%RSP%" echo "%%~ff"

set "SYSLIBS=kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ws2_32.lib winmm.lib imm32.lib ole32.lib uuid.lib"
link /nologo libcmt.lib @"%RSP%" ^
 "%VDIR%\imglibs.lib" "%VDIR%\lua514.lib" "%VDIR%\yyjson.lib" "%VDIR%\crijndael.lib" "%VDIR%\jpeg.lib" ^
 "%VDIR%\dr_mp3.lib" "%VDIR%\SDL3-static.lib" setupapi.lib version.lib shlwapi.lib ^
 "%VDIR%\freetype.lib" libvcruntime.lib libucrt.lib %SYSLIBS% ^
 /OUT:"%BUILD%\ragexe_scenes.exe" /SUBSYSTEM:WINDOWS /MACHINE:X64 /OPT:NOICF
if errorlevel 1 ( echo *** scene harness LINK FAILED *** & exit /b 1 )
echo Done -^> "%BUILD%\ragexe_scenes.exe"
