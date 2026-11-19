@echo off
REM tools\lossy_link\build.bat -> build\lossy_link.exe, the lossy link to play or test over
REM (tools\lossy_link\main.c says how to run it).
setlocal
set "ROOT=%~dp0..\.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\lossy_link\enet" mkdir "%ROOT%\build\lossy_link\enet"
pushd "%ROOT%\build\lossy_link" || exit /b 1
REM ENet as published, then the rig's files over it with warnings as errors.
cl /nologo /c /O2 /MD /I"%ROOT%\vendor\enet-1.3.18\include" "%ROOT%\vendor\enet-1.3.18\*.c" /Fo"enet\\"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W4 /WX /we4013 /O2 /MD /std:c11 /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\tools" /I"%ROOT%\vendor\enet-1.3.18\include" "%ROOT%\tools\lossy_link\main.c" "%ROOT%\tools\lossy_link\wire.c" "%ROOT%\tools\lossy_link\relay.c" enet\*.obj ws2_32.lib winmm.lib /Fe:"%ROOT%\build\lossy_link.exe"
set "CODE=%errorlevel%"
popd
exit /b %CODE%
