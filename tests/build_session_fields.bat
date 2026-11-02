@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\session_fields" mkdir "%ROOT%\build\tests\session_fields"
pushd "%ROOT%\build\tests\session_fields" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_session_fields.c" "%ROOT%\src\session\character_slots.c" "%ROOT%\src\game\shortcut_keys.c" /Fe:test_session_fields.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_session_fields.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
