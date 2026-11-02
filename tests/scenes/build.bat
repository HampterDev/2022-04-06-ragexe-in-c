@echo off
REM tests\scenes\build.bat -> build\ragexe_scenes.exe, the Windows client with the scene harness
REM (tests\scenes\README.md). It links the objects build.bat made, so run build.bat first.
setlocal
set "ROOT=%~dp0..\.."
set "BUILD=%ROOT%\build"
set "OBJ=%BUILD%\obj_scenes"
set "RSP=%BUILD%\ragexe_scenes.objs.rsp"
if not exist "%BUILD%\main_rebuild.objs.rsp" ( echo Run build.bat first. & exit /b 1 )
if not exist "%OBJ%" mkdir "%OBJ%"

call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set VSCMD_SKIP_SENDTELEMETRY=1

set "SCENES=%ROOT%\tests\scenes"
cl /nologo /std:c11 /c /O2 /Oy- /MD /GS /Gd /we4013 /source-charset:utf-8 /execution-charset:.949 ^
 /I"%ROOT%\src" /I"%SCENES%" /I"%ROOT%\vendor\stb" /I"%ROOT%\vendor\lua-5.1.4\src" ^
 "%SCENES%\scene_clock.c" "%SCENES%\scene_net.c" "%SCENES%\scene_thread.c" "%SCENES%\scene_settings.c" ^
 "%SCENES%\scene_input_win.c" "%SCENES%\scene_driver.c" /Fo:"%OBJ%\\"
if errorlevel 1 ( echo *** scene harness compilation FAILED *** & exit /b 1 )

REM The client's objects without the ones the harness replaces.
type nul > "%RSP%"
for /f "usebackq delims=" %%l in ("%BUILD%\main_rebuild.objs.rsp") do (
  set "skip="
  for %%n in (net clock thread drive_state settings window) do if /i "%%~nl"=="%%n" set "skip=1"
  if not defined skip >> "%RSP%" echo %%l
)
for %%f in ("%OBJ%\*.obj") do >> "%RSP%" echo "%%~ff"

set "VLIB=%BUILD%\vendor"
link /nologo msvcrt.lib msvcprt.lib @"%RSP%" ^
 "%VLIB%\imglibs.lib" "%VLIB%\lua514.lib" "%VLIB%\yyjson.lib" "%VLIB%\crijndael.lib" "%VLIB%\font_backend.lib" "%VLIB%\jpeg.lib" /LIBPATH:"%ROOT%\lib" ^
 cps.lib granny2.lib mss32.lib shlwapi.lib ^
 msvcp140.lib vcruntime.lib ucrt.lib legacy_stdio_definitions.lib ^
 kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib oleaut32.lib ^
 ws2_32.lib winmm.lib ddraw.lib d3d9.lib dxguid.lib gdiplus.lib imm32.lib ole32.lib uuid.lib netapi32.lib ^
 /OUT:"%BUILD%\ragexe_scenes.exe" /SUBSYSTEM:WINDOWS /MACHINE:X86 /SAFESEH /LARGEADDRESSAWARE /OPT:NOICF
if errorlevel 1 ( echo *** scene harness LINK FAILED *** & exit /b 1 )
echo Done -^> "%BUILD%\ragexe_scenes.exe"
