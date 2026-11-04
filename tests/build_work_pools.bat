@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\work_pools" mkdir "%ROOT%\build\tests\work_pools"
pushd "%ROOT%\build\tests\work_pools"
cl /nologo /W3 /WX /O2 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_work_pools.c" "%ROOT%\tests\work_pool_test_support.c" /Fe:test_work_pools.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_work_pools.exe
if errorlevel 1 (popd & exit /b 1)
if not errorlevel 0 (popd & exit /b 1)
popd
exit /b 0
