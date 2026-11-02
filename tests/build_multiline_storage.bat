@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\multiline_storage" mkdir "%ROOT%\build\tests\multiline_storage"
pushd "%ROOT%\build\tests\multiline_storage" || exit /b 1
cl /nologo /W4 /WX /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_multiline_storage.c" /Fe:test_multiline_storage.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_multiline_storage.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
