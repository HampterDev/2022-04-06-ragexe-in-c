@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\attack_records" mkdir "%ROOT%\build\tests\attack_records"
pushd "%ROOT%\build\tests\attack_records" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /Gy /Gw /MD /TC /std:c11 /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_attack_records.c" "%ROOT%\tests\attack_records_session.c" "%ROOT%\tests\attack_records_skill_fx.c" "%ROOT%\src\game\packets_act.c" "%ROOT%\src\game\skill_notify.c" "%ROOT%\src\actor\actor_msg.c" /Fe:test_attack_records.exe /link /OPT:REF winmm.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_attack_records.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
