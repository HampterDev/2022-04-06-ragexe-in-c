@echo off
REM tests\measure_requests.bat [loss=N] [delay=MS] [jitter=MS] [requests=N] [every=FRAMES] [frame=MS] [seed=N]
REM Builds and runs tests\measure_requests.c: how late a request reaches the server over a lossy
REM link, through the delivery piece and on the world channel. It measures and is no test of
REM the suite: it takes real time, a quarter of a minute with its own numbers.
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\measure_requests\enet" mkdir "%ROOT%\build\tests\measure_requests\enet"
pushd "%ROOT%\build\tests\measure_requests" || exit /b 1
REM ENet as published, then the client's and the rig's files over it with warnings as errors.
cl /nologo /c /O2 /MD /I"%ROOT%\vendor\enet-1.3.18\include" "%ROOT%\vendor\enet-1.3.18\*.c" /Fo"enet\\" >nul
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W4 /WX /we4013 /O2 /MD /std:c11 /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\tools" /I"%ROOT%\vendor\enet-1.3.18\include" "%ROOT%\tests\measure_requests.c" "%ROOT%\tools\lossy_link\wire.c" "%ROOT%\tools\lossy_link\relay.c" "%ROOT%\src\platform\net_enet.c" "%ROOT%\src\delivery\delivery.c" "%ROOT%\src\platform\net_win32.c" "%ROOT%\src\platform\clock_win32.c" "%ROOT%\src\platform\thread_win32.c" enet\*.obj ws2_32.lib winmm.lib user32.lib /Fe:measure_requests.exe >build.log
if not "%errorlevel%"=="0" (type build.log & popd & exit /b 1)
measure_requests.exe %*
set "CODE=%errorlevel%"
popd
exit /b %CODE%
