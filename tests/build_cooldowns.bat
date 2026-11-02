@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\cooldowns" mkdir "%ROOT%\build\tests\cooldowns"
pushd "%ROOT%\build\tests\cooldowns" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_cooldown_geometry.c" "%ROOT%\src\ui\cooldown_timer.c" /Fe:test_cooldown_geometry.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_cooldown_geometry.exe "%ROOT%\tests\fixtures\cooldown_geometry.bin"
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_cooldown_storage.c" "%ROOT%\src\ui\cooldown_timer.c" /Fe:test_cooldown_storage.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_cooldown_storage.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_cooldown_clock.c" "%ROOT%\src\ui\cooldown_lists.c" "%ROOT%\src\session\skill_delay.c" /Fe:test_cooldown_clock.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_cooldown_clock.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_cooldown_packets.c" "%ROOT%\src\ui\cooldown_timer.c" /Fe:test_cooldown_packets.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_cooldown_packets.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_cooldown_sheet.c" "%ROOT%\src\ui\cooldown_timer.c" /Fe:test_cooldown_sheet.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_cooldown_sheet.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
