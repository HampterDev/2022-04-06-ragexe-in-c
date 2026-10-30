@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\service_managers" mkdir "%ROOT%\build\tests\service_managers"
pushd "%ROOT%\build\tests\service_managers"
cl /nologo /W1 /WX /Gy /Gw /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_service_managers.c" "%ROOT%\tests\service_manager_support.c" "%ROOT%\src\web\party_booking_data.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" /Fe:test_service_managers.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_service_managers.exe
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
cl /nologo /W1 /WX /Gy /Gw /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_service_submissions.c" "%ROOT%\src\web\service_requests.c" "%ROOT%\src\session\character_name.c" "%ROOT%\src\web\service_manager_state.c" "%ROOT%\src\web\party_booking_data.c" "%ROOT%\src\core\owned_text.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" /Fe:test_service_submissions.exe /link /OPT:REF user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_service_submissions.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W1 /WX /Gy /Gw /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_party_request_snapshot.c" "%ROOT%\src\web\party_works.c" "%ROOT%\src\web\work_payloads.c" "%ROOT%\src\web\party_booking_data.c" "%ROOT%\src\core\owned_text.c" "%ROOT%\src\session\character_name.c" /Fe:test_party_request_snapshot.exe /link /OPT:REF winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_party_request_snapshot.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
