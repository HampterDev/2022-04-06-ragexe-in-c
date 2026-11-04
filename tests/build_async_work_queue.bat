@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\async_work_queue" mkdir "%ROOT%\build\tests\async_work_queue"
pushd "%ROOT%\build\tests\async_work_queue"
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_async_work_queue.c" /Fe:test_async_work_queue.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_async_work_queue.exe
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_async_submissions.c" "%ROOT%\src\web\requests.c" "%ROOT%\src\web\work_payloads.c" "%ROOT%\src\web\party_booking_data.c" /Fe:test_async_submissions.exe /link /OPT:REF winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_async_submissions.exe
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
popd
exit /b 0
