@echo off
REM tests\scenes\build.bat [freetype] -> build\ragexe_scenes.exe (ragexe_scenes_ft.exe), the Windows
REM client with the scene harness (tests\scenes\README.md). It links the objects build.bat made, so
REM run build.bat with the same arguments first.
setlocal enabledelayedexpansion
set "ROOT=%~dp0..\.."
set "BUILD=%ROOT%\build"
set "TEXT="
for %%a in (%*) do (
  if /i "%%~a"=="freetype" set "TEXT=freetype"
)
set "SUFFIX="
if defined TEXT set "SUFFIX=_ft"
set "VDIR=%BUILD%\vendor"
set "TEXTLIB="
set "TEXTDEF="
if defined TEXT (
  set "TEXTLIB="%VDIR%\freetype.lib""
  set "TEXTDEF=/DRAGE_TEXT_FREETYPE"
)
set "OBJ=%BUILD%\obj_scenes%SUFFIX%"
set "RSP=%BUILD%\ragexe_scenes%SUFFIX%.objs.rsp"
if not exist "%BUILD%\ragexe%SUFFIX%.objs.rsp" ( echo Run build.bat %TEXT% first. & exit /b 1 )
if not exist "%OBJ%" mkdir "%OBJ%"
REM Every object in OBJ is linked, so none may be left from a harness file that has gone.
del /q "%OBJ%\*.obj" 2>nul

call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set VSCMD_SKIP_SENDTELEMETRY=1

set "SCENES=%ROOT%\tests\scenes"
cl /nologo /std:c11 /c /O2 /Oy- /MT /GS /Gd /we4013 /source-charset:utf-8 /execution-charset:.949 %TEXTDEF% ^
 /I"%ROOT%\src" /I"%SCENES%" /I"%ROOT%\vendor\stb" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%VDIR%\sdl3-src\include" ^
 "%SCENES%\scene_clock.c" "%SCENES%\scene_net.c" "%SCENES%\scene_thread.c" "%SCENES%\scene_settings.c" ^
 "%SCENES%\scene_app_win.c" "%SCENES%\scene_window_win.c" "%SCENES%\scene_crash_win.c" "%SCENES%\scene_driver.c" /Fo:"%OBJ%\\"
if errorlevel 1 ( echo *** scene harness compilation FAILED *** & exit /b 1 )

REM The client's objects without the ones the harness replaces.
type nul > "%RSP%"
for /f "usebackq delims=" %%l in ("%BUILD%\ragexe%SUFFIX%.objs.rsp") do (
  set "skip="
  for %%n in (net_win32 clock_win32 thread_win32 drive_state settings_win32 app_sdl window_sdl crash_dialog) do if /i "%%~nl"=="%%n" set "skip=1"
  if not defined skip >> "%RSP%" echo %%l
)
for %%f in ("%OBJ%\*.obj") do >> "%RSP%" echo "%%~ff"

set "SYSLIBS=kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ws2_32.lib winmm.lib d3d9.lib gdiplus.lib imm32.lib ole32.lib uuid.lib"
link /nologo libcmt.lib @"%RSP%" ^
 "%VDIR%\imglibs.lib" "%VDIR%\lua514.lib" "%VDIR%\yyjson.lib" "%VDIR%\crijndael.lib" "%VDIR%\font_backend.lib" "%VDIR%\jpeg.lib" ^
 "%VDIR%\dr_mp3.lib" "%VDIR%\SDL3-static.lib" setupapi.lib version.lib shlwapi.lib ^
 %TEXTLIB% libvcruntime.lib libucrt.lib %SYSLIBS% ^
 /OUT:"%BUILD%\ragexe_scenes%SUFFIX%.exe" /SUBSYSTEM:WINDOWS /MACHINE:X64 /OPT:NOICF
if errorlevel 1 ( echo *** scene harness LINK FAILED *** & exit /b 1 )
echo Done -^> "%BUILD%\ragexe_scenes%SUFFIX%.exe"
