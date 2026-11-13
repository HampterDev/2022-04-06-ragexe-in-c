@echo off
setlocal
set "ROOT=%~dp0.."
set "OUT=%ROOT%\build\tests\camera_info"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
python "%ROOT%\tests\oracle\extract_functions.py" "%ROOT%" "%OUT%\camera_info.inc" ^
  "src/game/camera.c::line::#define WID_CAMERA_INFO 0x121" ^
  "src/game/camera.c::line::#pragma pack(push, 1)" ^
  "src/game/camera.c::struct PACKET_ZC_VIEW_CAMERAINFO {" ^
  "src/game/camera.c::line::#pragma pack(pop)" ^
  "src/game/camera.c::static int camera_value_within(" ^
  "src/game/camera.c::void game_mode_camera_info(" ^
  "src/world/view_camera.c::void view_process_camera("
if errorlevel 1 exit /b 1
pushd "%OUT%" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%OUT%" /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_camera_info.c" "%ROOT%\src\render\math3d.c" /Fe:test_camera_info.exe /link winmm.lib
if errorlevel 1 (popd & exit /b 1)
test_camera_info.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
