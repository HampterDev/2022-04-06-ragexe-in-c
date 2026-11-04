@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\npc_records" mkdir "%ROOT%\build\tests\npc_records"
pushd "%ROOT%\build\tests\npc_records" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_npc_records.c" "%ROOT%\src\ui\quests\npc_records.c" /Fe:test_npc_records.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_npc_records.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
