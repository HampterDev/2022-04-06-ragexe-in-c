@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\platform_net\enet" mkdir "%ROOT%\build\tests\platform_net\enet"
pushd "%ROOT%\build\tests\platform_net" || exit /b 1
REM ENet as published, then the client's files over it with warnings as errors.
cl /nologo /c /O2 /MD /I"%ROOT%\vendor\enet-1.3.18\include" "%ROOT%\vendor\enet-1.3.18\*.c" /Fo"enet\\"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W4 /WX /we4013 /O2 /MD /std:c11 /D_CRT_SECURE_NO_WARNINGS /D_WINSOCK_DEPRECATED_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\enet-1.3.18\include" "%ROOT%\tests\test_platform_net.c" "%ROOT%\src\platform\net_enet.c" "%ROOT%\src\delivery\delivery.c" "%ROOT%\src\platform\net_win32.c" "%ROOT%\src\platform\clock_win32.c" "%ROOT%\src\platform\thread_win32.c" enet\*.obj ws2_32.lib winmm.lib user32.lib /Fe:test_platform_net.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_platform_net.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
